#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include <libxml/parser.h>

#include <zmq.h>
#include "zhelpers.h"

#include "glb_config.h"
#include "glb_utils.h"
#include "glb_storage.h"
#include "glb_maze.h"
#include "glb_partition.h"


void *glbStorage_add_agent(void *arg)
{
    struct agent_args *args;
    struct glbStorage *storage;
    struct glbPartition *partition;
    struct glbMaze *maze;
    struct glbData *data;

    void *context;

    void *outbox;
    void *reg;

    char *confirm = NULL;
    size_t confirm_size = 0;

    char buf[GLB_TEMP_BUF_SIZE];

    const char *obj_id;
    int ret;

    ret = glbData_new(&data);
    if (ret != glb_OK) pthread_exit(NULL);

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

    reg = zmq_socket(context, ZMQ_REQ);
    if (!reg) pthread_exit(NULL);
    zmq_connect(reg, "inproc://register");


    while (1) {

	data->reset(data);
	buf[0] = '\0';

	printf("\n    ++ PARTITION AGENT #%d is ready to receive new tasks!\n", 
	       args->agent_id);

	data->spec = s_recv(outbox, &data->spec_size);


	if (strstr(data->spec, "ADD")) {

	    data->obj = s_recv(outbox, &data->obj_size);
	    data->text = s_recv(outbox, &data->text_size);
	    data->topics = s_recv(outbox, &data->topic_size);
	    data->index = s_recv(outbox, &data->index_size);

	    printf("    ++ PARTITION AGENT #%d has got spec \"%s\"\n", 
		   args->agent_id, data->spec);

	    printf("\nadding obj....\n\n");

	    ret = partition->add(partition, 
				 data);
	    if (ret != glb_OK) goto final;

	    printf("   new obj added: %s\n", 
		   data->local_id);

	    ret = maze->update(maze, 
			       data);
	    if (ret != glb_OK) goto final;

	    /* notify register */

	    sprintf(buf, "<spec action=\"add\" "
                         " obj_id=\"%s\""
                         " topics=\"%s\"/>\n",
		    data->id, data->metadata);

	    puts("    Notifying the Storage Registration Service...\n");

	    s_send(reg, buf, strlen(buf));
	    confirm = s_recv(reg, &confirm_size);

	    printf("    ++  PARTITION AGENT #%d: task complete!\n", 
		   args->agent_id);
	    goto final;

	}

	if (strstr(data->spec, "FIND")) {

	    printf("    ++  PARTITION AGENT #%d: FIND mode!\n", 
		   args->agent_id);

	    data->topics = s_recv(outbox, &data->topic_size);

	    printf(" receive topics: %s\n", data->topics);

	    data->interp = s_recv(outbox, &data->interp_size);

	    printf("    ++  PARTITION AGENT #%d: INTERP: %s\n", 
		   args->agent_id, data->interp);

	    ret = maze->search(maze, 
			       data);
	    if (ret != glb_OK) goto final;



	}


    final:

	fflush(stdout);
    }

    zmq_close(outbox);
    data->del(data);

    return NULL;
}


void *glbStorage_add_reception(void *arg)
{
    void *context;
    void *reception;
    void *agents;
    void *inbox;

    struct glbStorage *storage;
    struct glbData *data;

    int ret;

    storage = (struct glbStorage*)arg;
    context = storage->context;

    ret = glbData_new(&data);
    if (ret != glb_OK) pthread_exit(NULL);

    reception = zmq_socket(context, ZMQ_SUB);
    if (!reception) pthread_exit(NULL);

    zmq_connect(reception, "tcp://127.0.0.1:6911");
    zmq_setsockopt(reception, ZMQ_SUBSCRIBE, "", 0);

    inbox = zmq_socket(context, ZMQ_PUSH);
    if (!inbox) pthread_exit(NULL);
    zmq_connect(inbox, "inproc://inbox");

    while (1) {

	data->reset(data);

	printf("\n    ++ STORAGE RECEPTION is waiting for new tasks...\n");

        data->spec = s_recv(reception, &data->spec_size);

	if (strstr(data->spec, "ADD")) {

	    printf("\n    ++ STORAGE RECEPTION has got spec:\n"
                   "       %s\n", 
		   data->spec);

	    data->obj = s_recv(reception, &data->obj_size);
	    data->text = s_recv(reception, &data->text_size);
	    data->topics = s_recv(reception, &data->topic_size);
	    data->index = s_recv(reception, &data->index_size);


	    printf("\n    ++ STORAGE RECEPTION is sending task to Partitions..\n");
	    
	    s_sendmore(inbox, data->spec, data->spec_size);
	    s_sendmore(inbox, data->obj, data->obj_size);
	    s_sendmore(inbox, data->text, data->text_size);
	    s_sendmore(inbox, data->topics, data->topic_size);
	    s_send(inbox, data->index, data->index_size);

	    goto final;
	}

	if (strstr(data->spec, "FIND")) {

	    printf("receiving FIND arguments...\n");
	    /*interp = s_recv(reception, &data->interp_size);

	    s_sendmore(inbox, spec, spec_size);
	    s_send(inbox, interp, interp_size);*/

	}

    final:

        fflush(stdout);
    }

    /* we never get here */
    zmq_close(reception);
    zmq_term(context);

    return;
}

void *glbStorage_add_registration(void *arg)
{
    void *context;
    void *reg;
    void *control;

    struct glbStorage *storage;
    struct glbData *data;

    char *reply;
    size_t reply_size = 0;

    const char *err_msg;
    size_t err_msg_size = 0;

    char buf[GLB_TEMP_BUF_SIZE];

    int ret;

    ret = glbData_new(&data);
    if (ret != glb_OK) pthread_exit(NULL);

    storage = (struct glbStorage*)arg;
    context = storage->context;

    reg = zmq_socket(context, ZMQ_REP);
    if (!reg) pthread_exit(NULL);
    zmq_bind(reg, "inproc://register");

    control = zmq_socket(context, ZMQ_PUSH);
    if (!control) pthread_exit(NULL);
    ret = zmq_connect(control, "tcp://127.0.0.1:5561");

    while (1) {

	data->reset(data);

	printf("\n    ++ STORAGE REGISTRATION is waiting for new tasks...\n");

        data->spec = s_recv(reg, &data->spec_size);

	printf("    ++ STORAGE REGISTRATION has got spec:\n"
               "       %s\n", data->spec);

	ret = storage->process(storage, data);

	/* notify controller */
	if (data->control_msg) {
	    s_send(control, (const char*)data->control_msg, 
		   data->control_msg_size);
	}

	if (data->reply) {
	    s_send(reg, data->reply, data->reply_size);
	}
	else {
	    err_msg = "{\"error\": \"incorrect call\"}";
	    err_msg_size = strlen(err_msg);
	    s_send(reg, err_msg, err_msg_size);
	}


        fflush(stdout);
    }

    /* we never get here */
    zmq_close(reg);
    zmq_term(context);

    return;
}


void *glbStorage_add_metadata_service(void *arg)
{
    void *context;
    void *reg;
    void *coll;

    struct glbStorage *storage;
    struct glbData *data;

    int ret;

    storage = (struct glbStorage*)arg;
    context = storage->context;

    ret = glbData_new(&data);
    if (ret != glb_OK) pthread_exit(NULL);


    reg = zmq_socket(context, ZMQ_REQ);
    if (!reg) pthread_exit(NULL);
    zmq_connect(reg, "inproc://register");

    coll = zmq_socket(context, ZMQ_REP);
    if (!coll) pthread_exit(NULL);
    zmq_connect(coll, "tcp://127.0.0.1:6903");

    while (1) {

	data->reset(data);

	printf("\n    ++ STORAGE METADATA SERVICE is waiting for new tasks...\n");

	/* get the query */
        data->query = s_recv(coll, &data->query_size);

	printf("    ++ STORAGE METADATA SERVICE has got query:\n"
               "       %s\n", data->query);

        /* metadata query */
	/* check the registration */
        s_send(reg, data->query, data->query_size);
        data->reply = s_recv(reg, &data->reply_size);


	s_send(coll, data->reply, data->reply_size);

    final:

        fflush(stdout);
    }

    /* we never get here */
    zmq_close(reg);
    zmq_term(context);

    return;
}

void *glbStorage_add_search_service(void *arg)
{
    void *context;
    void *coll;
    void *inbox;

    struct glbStorage *storage;
    struct glbData *data;

    int ret;

    storage = (struct glbStorage*)arg;
    context = storage->context;

    ret = glbData_new(&data);
    if (ret != glb_OK) pthread_exit(NULL);

    coll = zmq_socket(context, ZMQ_PULL);
    if (!coll) pthread_exit(NULL);
    zmq_connect(coll, "tcp://127.0.0.1:6905");

    inbox = zmq_socket(context, ZMQ_PUSH);
    if (!inbox) pthread_exit(NULL);
    zmq_connect(inbox, "inproc://inbox");

    while (1) {

	data->reset(data);

	printf("\n    ++ STORAGE SEARCH SERVICE is waiting for new tasks...\n");

	/* get the query */
        data->spec = s_recv(coll, &data->spec_size);
        data->topics = s_recv(coll, &data->topic_size);
	data->interp = s_recv(coll, &data->interp_size);

	printf("    ++ STORAGE SEARCH SERVICE has got query:\n"
               "       %s\n", data->interp);

        /* search query */
        s_sendmore(inbox, data->spec, data->spec_size);
        s_sendmore(inbox, data->topics, data->topic_size);
        s_send(inbox, data->interp, data->interp_size);

    final:

        fflush(stdout);
    }

    /* we never get here */
    zmq_close(coll);
    zmq_close(inbox);
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

    pthread_t reception;
    pthread_t registration;
    pthread_t metadata_service;
    pthread_t search_service;

    pthread_t agents[GLB_NUM_AGENTS];
    pthread_t *agent;

    struct agent_args a_args[GLB_NUM_AGENTS];

    xmlInitParser();

    ret = glbStorage_new(&storage, config);
    if (ret) {
        fprintf(stderr, "Couldn\'t load glbStorage... ");
        return -1;
    }
    storage->name = "LYNX 172.20.41.18";

    context = zmq_init(1);
    storage->context = context;

    /* add a reception */
    ret = pthread_create(&reception,
			 NULL,
			 glbStorage_add_reception,
			 (void*)storage);

    /* add a registration service */
    ret = pthread_create(&registration,
			 NULL,
			 glbStorage_add_registration,
			 (void*)storage);

    /* add metadata service */
    ret = pthread_create(&metadata_service,
			 NULL,
			 glbStorage_add_metadata_service,
			 (void*)storage);

    /* add search service */
    ret = pthread_create(&search_service,
			 NULL,
			 glbStorage_add_search_service,
			 (void*)storage);

    /* add agents */
    for (i = 0; i < GLB_NUM_AGENTS; i++) {
	a_args[i].agent_id = i; 
 	a_args[i].storage = storage; 
        agent = &agents[i];
        ret = pthread_create(agent, 
			     NULL,
			     glbStorage_add_agent, 
			     (void*)&a_args[i]);
    }


    storage->start(storage);

    return 0;
}


