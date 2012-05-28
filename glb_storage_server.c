#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include <libxml/parser.h>

#include <zmq.h>
#include "zhelpers.h"

#include "glb_config.h"
#include "glb_storage.h"
#include "glb_maze.h"
#include "glb_partition.h"


void *glbStorage_add_agent(void *arg)
{
    struct agent_args *args;
    struct glbStorage *storage;
    struct glbPartition *partition;
    struct glbMaze *maze;

    void *context;
    void *outbox;

    char *spec = NULL;
    void *obj = NULL;
    char *text = NULL;
    char *interp = NULL ;

    size_t spec_size = 0;
    size_t obj_size = 0;
    size_t text_size = 0;
    size_t interp_size = 0;

    const char *obj_id;
    int ret;

    args = (struct agent_args*)arg;

    storage = args->storage;
    context = storage->context;

    maze = storage->mazes[args->agent_id];
    if (!maze) {
	printf("No maze for agent %d :(\n", args->agent_id);
	pthread_exit(NULL);
    }

    partition = storage->partitions[args->agent_id];
    if (!partition) {
	printf("No partition for agent %d :(\n", args->agent_id);
	pthread_exit(NULL);
    }

    /* get messages from outbox */
    outbox = zmq_socket(context, ZMQ_PULL);
    if (!outbox) pthread_exit(NULL);
    ret = zmq_connect(outbox, "inproc://outbox");

    while (1) {

	spec_size = 0;
	obj_size = 0;
	text_size = 0;
	interp_size = 0;

	spec = NULL;
	obj = NULL;
	text = NULL;
	interp = NULL;

	printf(" ++ agent %d waiting...\n", 
	       args->agent_id);

	spec = s_recv(outbox, &spec_size);

	printf("SPEC: %s\n", spec);

	if (!strcmp(spec, "ADD")) {

	    text = s_recv(outbox, &text_size);
	    interp = s_recv(outbox, &interp_size);

	    printf("agent #%d got ADD spec\n", 
		   args->agent_id);

	    ret = partition->add(partition, 
				 spec,
				 spec_size,
				 obj,
				 obj_size,
				 text,
				 text_size,
				 &obj_id);
	    if (ret != glb_OK) goto err;

	    ret = maze->read(maze, (const char*)interp,
		       interp_size, obj_id);
	    goto final;
	}

	if (!strcmp(spec, "FIND")) {
	    interp = s_recv(outbox, &interp_size);
	    printf("agent #%d got FIND spec\n", 
		   args->agent_id);

	}

    err: 
	if (spec) free(spec);
	if (text) free(text);
	if (interp) free(interp);

    final:
	fflush(stdout);
    }

    zmq_close(outbox);

    return NULL;
}


void *glbStorage_add_reception(void *arg)
{
    void *context;
    void *reception;
    void *agents;
    void *inbox;
    char *spec, *obj, *interp;

    size_t spec_size = 0;
    size_t obj_size = 0;
    size_t interp_size = 0;

    int ret;

    context = arg;

    reception = zmq_socket(context, ZMQ_SUB);
    if (!reception) pthread_exit(NULL);

    zmq_connect(reception, "tcp://127.0.0.1:6911");
    zmq_setsockopt(reception, ZMQ_SUBSCRIBE, "", 0);

    inbox = zmq_socket(context, ZMQ_PUSH);
    if (!inbox) pthread_exit(NULL);
    zmq_connect(inbox, "inproc://inbox");

    while (1) {

	spec = NULL;
	obj = NULL;
	interp = NULL;

	spec_size = 0;
	obj_size = 0;
	interp_size = 0;

	printf("   Storage is waiting for specs...\n");

        spec = s_recv(reception, &spec_size);

	printf("   Storage got spec: %s\n", spec);

	if (!strcmp(spec, "ADD")) {

	    printf("receiving ADD arguments...\n");

	    obj = s_recv(reception, &obj_size);
	    interp = s_recv(reception, &interp_size);

	    printf("%s %s\n", obj, interp);
	    
	    printf("sending...\n");

	    s_sendmore(inbox, spec, spec_size);
	    s_sendmore(inbox, obj, obj_size);
	    s_send(inbox, interp, interp_size);
	    goto final;
	}

	if (!strcmp(spec, "FIND")) {

	    printf("receiving FIND arguments...\n");
	    interp = s_recv(reception, &interp_size);

	    s_sendmore(inbox, spec, spec_size);
	    s_send(inbox, interp, interp_size);
	}

    final:
	if (spec) free(spec);
	if (obj) free(obj);
	if (interp) free(interp);

        fflush(stdout);
    }

    /* we never get here */
    zmq_close(reception);
    zmq_term(context);

    return;
}



int 
main(int           const argc, 
     const char ** const argv) 
{
    struct glbStorage *storage;
    const char *config = "storage.ini";
    void *context;
    int i, ret;

    pthread_t reception, agent;

    struct agent_args a_args[GLB_NUM_AGENTS];

    xmlInitParser();


    ret = glbStorage_new(&storage, config);
    if (ret) {
        fprintf(stderr, "Couldn\'t load glbStorage... ");
        return -1;
    }

    context = zmq_init(1);
    storage->context = context;

    /* add a reception */
    ret = pthread_create(&reception,
			 NULL,
			 glbStorage_add_reception,
			 (void*)context);

    /* add agents */
    for (i = 0; i < GLB_NUM_AGENTS; i++) {
	a_args[i].agent_id = i; 
 	a_args[i].storage = storage; 
        ret = pthread_create(&agent, 
			     NULL,
			     glbStorage_add_agent, 
			     (void*)&a_args[i]);
    }

    /*    printf("storage: %p\n", storage->start);*/

    storage->start(storage);

    return 0;
}


