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

    struct glbCollection *collection;
    void *zmq_context;
};

void *worker_routine(void *arg)
{
    struct worker_args *args;
    struct glbCollection *collection;
    void *client;
    void *outbox;
    char *msg;
    char *result;
    int ret;

    args = (struct worker_args*)arg;
    collection = args->collection;

    client = zmq_socket(args->zmq_context, ZMQ_REP);
    if (!client) pthread_exit(NULL);
    ret = zmq_connect(client, "tcp://127.0.0.1:6909");

    while (1) {
        msg = s_recv(client);
        printf("Worker #%d: processing message \"%s\"...\n", 
            args->worker_id, msg);

        result = glbCollection_process((void*)collection, (const char*)msg);
        printf("RESULT: %s\n", result);

        s_send(client, result);

        fflush(stdout);
        free(msg);
        //glbCollection_free_result(result);
    }
    zmq_close(client);

    /*zmq_close(outbox);*/
}


int 
main(int           const argc, 
     const char ** const argv) 
{
    void *context;
    void *frontend;
    void *backend;

    struct glbCollection *collection;
    //const char *config_filename;
    void *receiver;
    void *sender;
    int i, ret;

    struct worker_args w_args[NUM_WORKERS];

    /* initialize poll set */
    zmq_pollitem_t items [] = {
      { frontend, 0, ZMQ_POLLIN, 0 },
      { backend, 0, ZMQ_POLLIN, 0 }
    };

    //if (argc-1 != 1) {
    //    fprintf(stderr, "You must specify 1 argument:  "
    //            " the name of the configuration file. "
    //            "You specified %d arguments.\n",  argc-1);
    //    exit(1);
    //}
    //config_filename = argv[1];
        
    ret = glbCollection_new(&collection);
    if (ret) {
        fprintf(stderr, "Couldn\'t load glbCollection... ");
        return -1;
    }
    

    /*  one I/O thread in the thread pool will do. */
    context = zmq_init(1);

    frontend = zmq_socket(context, ZMQ_ROUTER);
    backend = zmq_socket(context, ZMQ_DEALER);
    zmq_bind(frontend, "tcp://127.0.0.1:6908");
    zmq_bind(backend, "tcp://127.0.0.1:6909");

    /* pool of threads */
    for (i = 0; i < NUM_WORKERS; i++) {
        pthread_t worker;

        w_args[i].worker_id = i; 
        w_args[i].collection = collection; 
        w_args[i].zmq_context = context;
 
        ret = pthread_create(&worker, NULL, worker_routine, (void*)&w_args[i]);
    }

    zmq_device(ZMQ_QUEUE, frontend, backend);

    /* we never get here */
    zmq_close(frontend);
    zmq_close(backend);
    zmq_term(context);
    return 0;
}


