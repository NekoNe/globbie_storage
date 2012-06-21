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
    void *delivery;

    char *confirm = NULL;
    size_t confirm_size = 0;

    char buf[GLB_TEMP_BUF_SIZE];

    const char *obj_id;
    const char *ticket;
    int ret;

    const char *no_results_msg = "<div class=\\\"msg_row\\\">"
                         "<div class=\\\"msg_row_title\\\">"
                         " К сожалению, по данному запросу ничего не найдено.</div></div>";
    size_t no_results_msg_size = strlen(no_results_msg);

    const char *msg;
    size_t msg_size;

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

    delivery = zmq_socket(context, ZMQ_REQ);
    if (!delivery) pthread_exit(NULL);
    zmq_connect(delivery, "tcp://127.0.0.1:6902");


    while (1) {

	data->reset(data);
	buf[0] = '\0';

	printf("\n    ++ PARTITION AGENT #%d is ready to receive new tasks!\n", 
	       args->agent_id);

	data->spec = s_recv(outbox, &data->spec_size);
	data->interp = s_recv(outbox, &data->interp_size);
	data->text = s_recv(outbox, &data->text_size);
	data->topics = s_recv(outbox, &data->topic_size);
	data->index = s_recv(outbox, &data->index_size);

	if (strstr(data->spec, "ADD")) {
	    printf("    ++ PARTITION AGENT #%d has got spec \"%s\"\n", 
		   args->agent_id, data->spec);

	    ret = partition->add(partition, 
				 data);
	    if (ret != glb_OK) goto final;

	    printf("    ++ obj saved with local id: %s\n", 
		   data->local_id);

	    /*ret = maze->read_index(maze, (const char*)data->local_id);
	      if (ret != glb_OK) goto final; */


	    /* notify delivery service */
	    
	    /*
	    sprintf(buf, "<spec action=\"add_meta\" "
                         " obj_id=\"%s\""
                         " topics=\"%s\"/>\n",
		    data->id, data->metadata);

	    puts("    !! notifying the Delivery Service...\n");
	    s_sendmore(reg, buf, strlen(buf));
	    s_send(reg, "<noobj/>", strlen("<noobj/>"));
	    confirm = s_recv(reg, &confirm_size); */

	    printf("    ++  PARTITION AGENT #%d: task complete!\n", 
		   args->agent_id);

	    goto final;

	}

	if (strstr(data->spec, "FIND")) {
	    printf("    ++  PARTITION AGENT #%d: got spec %s\n", 
		   args->agent_id, data->spec);

	    ticket = strstr(data->spec, " ");
	    ticket++;
	    data->ticket = strdup(ticket); 
	    if (!data->ticket) goto final;
	    
	    ret = maze->search(maze, 
			       data);

	    if (ret != glb_OK) {
		msg = no_results_msg;
		msg_size = no_results_msg_size;
	    }
	    else {
		msg = (const char*)maze->results;
		msg_size = maze->results_size;
	    }

	    /* creating spec */
	    sprintf(buf, "<spec action=\"add_search_results\" "
		    "  ticket=\"%s\"/>\n",
		    data->ticket);

	    printf("    !! notifying the Delivery Service...: %s\n", msg);

	    s_sendmore(delivery, (const char*)buf, strlen(buf));
	    s_send(delivery, msg, msg_size);

	    confirm = s_recv(delivery, &confirm_size);

	    printf("    ++  PARTITION AGENT #%d:\n"
                   "        delivery confirm: %s\n"
                   "        task complete!\n", 
		   args->agent_id, confirm);
	}


    final:

	/*if (confirm) {
	    free(confirm);
	    printf("del OK!\n");
	    }*/

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

	printf("\n    ++ STORAGE RECEPTION has got spec:\n"
                   "       %s\n",  data->spec);

	data->interp = s_recv(reception, &data->interp_size);
	data->text = s_recv(reception, &data->text_size);
	data->topics = s_recv(reception, &data->topic_size);
	data->index = s_recv(reception, &data->index_size);

	printf("\n    ++ STORAGE RECEPTION is sending task to Partitions..\n");
      
	s_sendmore(inbox, data->spec, data->spec_size);
	s_sendmore(inbox, data->interp, data->interp_size);
	s_sendmore(inbox, data->text, data->text_size);
	s_sendmore(inbox, data->topics, data->topic_size);
	s_send(inbox, data->index, data->index_size);

	printf("    ++ all messages sent!\n");

        fflush(stdout);
    }

    /* we never get here */
    zmq_close(reception);
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
        data->spec = s_recv(coll, &data->spec_size);

	printf("got spec: %s\n\n", data->spec);

	data->interp = s_recv(coll, &data->interp_size);

	printf("got interp: %s\n\n", data->interp);

	data->text = s_recv(coll, &data->text_size);

	printf("got text: %s\n\n", data->text);

	data->topics = s_recv(coll, &data->topic_size);

	printf("got topics: %s\n\n", data->topics);

	data->index = s_recv(coll, &data->index_size);

	printf("\n    ++ STORAGE Search Service is sending task to Partitions..\n");
	    

	s_sendmore(inbox, data->spec, data->spec_size);
	s_sendmore(inbox, data->interp, data->interp_size);
	s_sendmore(inbox, data->text, data->text_size);
	s_sendmore(inbox, data->topics, data->topic_size);
	s_send(inbox, data->index, data->index_size);

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
main(int const argc, 
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

    /*xmlInitParser();*/

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
    /*ret = pthread_create(&registration,
			 NULL,
			 glbStorage_add_registration,
			 (void*)storage);*/

    /* add metadata service */
    /*ret = pthread_create(&metadata_service,
			 NULL,
			 glbStorage_add_metadata_service,
			 (void*)storage);*/

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


