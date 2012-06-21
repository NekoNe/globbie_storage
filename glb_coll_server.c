#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>


#include <zmq.h>
#include "zhelpers.h"

#include "glb_config.h"
#include "glb_collection.h"
#include "glb_utils.h"

#define NUM_WORKERS 1

struct worker_args {
    int worker_id;

    struct glbColl *collection;
    void *zmq_context;
};

void *worker_routine(void *arg)
{
    struct worker_args *args;
    struct glbColl *coll;

    void *client;
    void *publisher;
    void *search;

    const char *dest_coll_addr = NULL;
    const char *result;

    struct glbData *data;

    int ret;
    char buf[1024];

    args = (struct worker_args*)arg;
    coll = args->collection;

    ret = glbData_new(&data);
    if (ret != glb_OK) pthread_exit(NULL);

    client = zmq_socket(args->zmq_context, ZMQ_PULL);
    if (!client) pthread_exit(NULL);
    ret = zmq_connect(client, "tcp://127.0.0.1:6909");

    publisher = zmq_socket(args->zmq_context, ZMQ_PUB);
    if (!publisher) pthread_exit(NULL);
    zmq_connect(publisher, "tcp://127.0.0.1:6910");

    search = zmq_socket(args->zmq_context, ZMQ_PUSH);
    if (!search) pthread_exit(NULL);
    ret = zmq_connect(search, "tcp://127.0.0.1:6904");

    while (1) {

	data->reset(data);

	/* waiting for spec */
        data->spec = s_recv(client, &data->spec_size);
	data->obj = s_recv(client, &data->obj_size);
	data->text = s_recv(client, &data->text_size);
	data->topics = s_recv(client, &data->topic_size);
	data->index = s_recv(client, &data->index_size);


	printf("    !! iCollection Reception #%d: got spec \"%s\"\n", 
	       args->worker_id, data->spec);

	ret = coll->find_route(coll, data->topics, &dest_coll_addr);

	/* stay in this collection */
	if (!dest_coll_addr) {
	    s_sendmore(publisher, data->spec, data->spec_size);
	    s_sendmore(publisher, data->obj, data->obj_size);
	    s_sendmore(publisher, data->text, data->text_size);
	    s_sendmore(publisher, data->topics, data->topic_size);
	    s_send(publisher, data->index, data->index_size);
	}

        fflush(stdout);
    }

    zmq_close(client);


    return;
}

void *glbColl_add_delivery_service(void *arg)
{
    void *context;
    void *frontend;
    void *backend;
    int ret;

    context = zmq_init(1);

    frontend = zmq_socket(context, ZMQ_ROUTER);
    backend = zmq_socket(context, ZMQ_DEALER);

    zmq_bind(frontend, "tcp://127.0.0.1:6902");
    zmq_bind(backend, "tcp://127.0.0.1:6903");

    printf("    ++ Root iCollection Delivery Service is up and running!...\n\n");

    zmq_device(ZMQ_QUEUE, frontend, backend);

    /* we never get here */
    zmq_close(frontend);
    zmq_close(backend);
    zmq_term(context);
    return;
}


void *glbColl_add_search_service(void *arg)
{
    void *context;
    void *frontend;
    void *backend;
    int ret;

    context = zmq_init(1);

    frontend = zmq_socket(context, ZMQ_PULL);
    backend = zmq_socket(context, ZMQ_PUSH);

    zmq_bind(frontend, "tcp://127.0.0.1:6904");
    zmq_bind(backend, "tcp://127.0.0.1:6905");

    printf("    ++ Root iCollection Search Service is up and running!...\n\n");

    zmq_device(ZMQ_QUEUE, frontend, backend);

    /* we never get here */
    zmq_close(frontend);
    zmq_close(backend);
    zmq_term(context);
    return;
}
  

int 
main(int           const argc, 
     const char ** const argv) 
{
    void *context;
    void *publisher;
    struct glbColl *coll;

    pthread_t worker;
    pthread_t delivery_service;
    pthread_t search_service;

    struct worker_args w_args[NUM_WORKERS];
    const char *config = "coll.ini";
    int i, ret;

    ret = glbColl_new(&coll, config);
    if (ret) {
        fprintf(stderr, "Couldn\'t load glbColl... ");
        return -1;
    }

    coll->name = "ROOT";

    context = zmq_init(1);

    coll->context = context;

    /* add delivery service */
    ret = pthread_create(&delivery_service,
			 NULL,
			 glbColl_add_delivery_service,
			 (void*)coll);

    /* add search service */
    ret = pthread_create(&search_service,
			 NULL,
			 glbColl_add_search_service,
			 (void*)coll);

    /* pool of threads */
    for (i = 0; i < NUM_WORKERS; i++) {
        w_args[i].worker_id = i; 
        w_args[i].collection = coll; 
        w_args[i].zmq_context = context;
 
        ret = pthread_create(&worker, NULL, worker_routine, (void*)&w_args[i]);
    }

    coll->start(coll);

    /* we never get here */
    zmq_term(context);

    return 0;
}


