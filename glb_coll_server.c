#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>


#include <zmq.h>
#include "zhelpers.h"

#include "glb_config.h"
#include "glb_collection.h"

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
    const char *dest_coll_addr = NULL;
    const char *result;

    char *spec;
    char *obj; 
    char *text; 
    char *interp;
    char *topics;
    char *index;

    size_t spec_size = 0;
    size_t obj_size  = 0;
    size_t text_size = 0;
    size_t interp_size = 0;
    size_t topic_size = 0;
    size_t index_size = 0;

    int ret;
    char buf[1024];

    args = (struct worker_args*)arg;
    coll = args->collection;

    client = zmq_socket(args->zmq_context, ZMQ_PULL);
    if (!client) pthread_exit(NULL);
    ret = zmq_connect(client, "tcp://127.0.0.1:6909");


    publisher = zmq_socket(args->zmq_context, ZMQ_PUB);
    if (!publisher) pthread_exit(NULL);
    zmq_connect(publisher, "tcp://127.0.0.1:6910");

    while (1) {

	spec_size = 0;
	text_size = 0;
	obj_size = 0;
	interp_size = 0;
	topic_size = 0;
	index_size = 0;

        spec = s_recv(client, &spec_size);
	obj = s_recv(client, &obj_size);
	text = s_recv(client, &text_size);
	topics = s_recv(client, &topic_size);
	index = s_recv(client, &index_size);

        printf("    !! iCollection Reception #%d: got spec \"%s\"\n", 
	       args->worker_id, spec);

	ret = coll->find_route(coll, topics, &dest_coll_addr);

	/* stay in this collection */
	if (!dest_coll_addr) {
	    s_sendmore(publisher, spec, spec_size);
	    s_sendmore(publisher, obj, obj_size);
	    s_sendmore(publisher, text, text_size);
	    s_sendmore(publisher, topics, topic_size);
	    s_send(publisher, index, index_size);
	}

	free(spec);
        free(obj);
	free(text);
        free(topics);
        free(index);

        fflush(stdout);
    }

    zmq_close(client);


    return;
}

 void *glbColl_add_search_service(void *arg)
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


