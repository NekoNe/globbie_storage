#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>


#include <zmq.h>
#include "zhelpers.h"

#include "glb_config.h"
#include "glb_collection.h"

#define NUM_WORKERS 4

struct worker_args {
    int worker_id;

    struct glbColl *collection;
    void *zmq_context;
};

void *worker_routine(void *arg)
{
    struct worker_args *args;
    struct glbColl *collection;
    void *client;
    void *outbox;
    char *header, *obj, *interp;
    const char *result;
    int ret;

    args = (struct worker_args*)arg;
    collection = args->collection;

    client = zmq_socket(args->zmq_context, ZMQ_PULL);
    if (!client) pthread_exit(NULL);
    ret = zmq_connect(client, "tcp://127.0.0.1:6902");

    while (1) {

        header = s_recv(client);

        printf("  Collection Input Manager #%d: got header \"%s\"...\n", 
                args->worker_id, header);

	obj = s_recv(client);
	interp = s_recv(client);


        printf("  Collection Input Manager #%d: orig: \"%s\"\n  interp: \"%s\"...\n", 
	       args->worker_id, obj, interp);

	result = glbColl_process((void*)collection,
				 (const char*)interp);

        printf("RESULT: %s\n", result);

	/*s_send(client, result);*/
	free(header);
        free(obj);
        free(interp);

        fflush(stdout);
    }
    zmq_close(client);

}


int 
main(int           const argc, 
     const char ** const argv) 
{
    void *context;
    void *frontend;
    void *backend;

    struct glbColl *collection;
    //const char *config_filename;
    void *receiver;
    void *sender;
    int i, ret;

    struct worker_args w_args[NUM_WORKERS];

    //if (argc-1 != 1) {
    //    fprintf(stderr, "You must specify 1 argument:  "
    //            " the name of the configuration file. "
    //            "You specified %d arguments.\n",  argc-1);
    //    exit(1);
    //}
    //config_filename = argv[1];

    ret = glbColl_new(&collection);
    if (ret) {
        fprintf(stderr, "Couldn\'t load glbColl... ");
        return -1;
    }
    

    /*  one I/O thread in the thread pool will do. */
    context = zmq_init(1);

    frontend = zmq_socket(context, ZMQ_PULL);
    backend = zmq_socket(context, ZMQ_PUSH);
    zmq_bind(frontend, "tcp://127.0.0.1:6901");
    zmq_bind(backend, "tcp://127.0.0.1:6902");

    /* pool of threads */
    for (i = 0; i < NUM_WORKERS; i++) {
        pthread_t worker;

        w_args[i].worker_id = i; 
        w_args[i].collection = collection; 
        w_args[i].zmq_context = context;
 
        ret = pthread_create(&worker, NULL, worker_routine, (void*)&w_args[i]);
    }

    printf("  ++ Globbie server is up and running!...\n\n");

    zmq_device(ZMQ_QUEUE, frontend, backend);

    /* we never get here */
    zmq_close(frontend);
    zmq_close(backend);
    zmq_term(context);
    return 0;
}


