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
    char *spec, *obj, *interp;
    const char *dest_coll_addr = NULL;
    const char *result;
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

    /*sleep(1); */
    /*s_send(publisher, "OK");
    s_send(publisher, "OK NEXT");
    printf("publishing done!\n"); */

    while (1) {

        spec = s_recv(client);
	obj = s_recv(client);
	interp = s_recv(client);

        printf("  Coll Reception #%d: got spec \"%s\"...\n", 
                args->worker_id, spec);
        printf("  Coll Reception #%d: orig: \"%s\"\n  interp: \"%s\"...\n", 
	       args->worker_id, obj, interp);

	/*result = glbColl_process((void*)collection,
	  (const char*)interp);*/
	ret = coll->find_route(coll, spec, &dest_coll_addr);

	/* stay in this collection */
	if (!dest_coll_addr) {

	    s_sendmore(publisher, spec);
	    s_sendmore(publisher, obj);
	    s_send(publisher, interp);
	}

	free(spec);
        free(obj);
        free(interp);

        fflush(stdout);
    }

    zmq_close(client);


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
    struct worker_args w_args[NUM_WORKERS];
    const char *config = "coll.ini";
    int i, ret;

    ret = glbColl_new(&coll, config);
    if (ret) {
        fprintf(stderr, "Couldn\'t load glbColl... ");
        return -1;
    }

    coll->name = "ROOT";

    /*  one I/O thread in the thread pool will do. */
    context = zmq_init(1);

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


