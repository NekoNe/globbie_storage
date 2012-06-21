#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <time.h>

#include <libxml/parser.h>

#include <zmq.h>
#include "zhelpers.h"

#include "glb_config.h"
#include "glb_delivery.h"

static int glbDelivery_add_meta(struct glbDelivery *self,
			       struct glbData *data);

static int glbDelivery_add_search_results(struct glbDelivery *self,
			       struct glbData *data);

static int
glbDelivery_str(struct glbDelivery *self)
{
    printf("<struct glbDelivery at %p>\n", self);


    return glb_OK;
}


static int
glbDelivery_del(struct glbDelivery *self)
{

    free(self);
    return glb_OK;
}


static int
glbDelivery_get_meta(struct glbDelivery *self, 
		    struct glbData *data)
{
    const char *meta;
    const char *tag_begin = "{\"topic\": \"";
    const char *tag_close = "\"}";
    char *buf;
    int i, ret;

    if (!data->id) return glb_FAIL;


    meta = (const char*)self->obj_index->get(self->obj_index, 
					     (const char*)data->id);
    if (!meta) return glb_FAIL;

    data->reply_size = strlen(tag_begin) +\
	strlen(meta) +  strlen(tag_close);

    data->reply = malloc(data->reply_size + 1);

    if (!data->reply) return glb_FAIL;

    buf = data->reply;
    memcpy(buf, tag_begin, strlen(tag_begin));
    buf += strlen(tag_begin);

    memcpy(buf, meta, strlen(meta));
    buf += strlen(meta);

    memcpy(buf, tag_close, strlen(tag_close));
    buf += strlen(tag_close);

    *buf = '\0';

    return glb_OK;
}



static int
glbDelivery_get_search_results(struct glbDelivery *self, 
			      struct glbData *data)
{
    struct glbResult *res;
    int i, ret;

    if (!data->ticket) return glb_FAIL;

    res = (struct glbResult*)self->search_cache->get\
	(self->search_cache, 
	 (const char*)data->ticket);
    if (!res) return glb_NEED_WAIT;

    printf("  RESULTS in Dict: %s\n", res->text);

    data->reply = malloc(res->text_size + 1);
    if (!data->reply) return glb_FAIL;

    memcpy(data->reply, res->text, res->text_size);
    data->reply_size = res->text_size;
    data->reply[res->text_size] = '\0';

    return glb_OK;
}



static int
glbDelivery_add_meta(struct glbDelivery *self, 
		    struct glbData *data)
{
    char buf[GLB_TEMP_BUF_SIZE];
    int ret;

    printf("\n\n    Delivery Register ADD metadata for \"%s\": \"%s\"\n", 
	   data->id, data->metadata);

    ret = self->obj_index->set(self->obj_index, 
			       (const char*)data->id, 
			       (void*)strdup(data->metadata));
    if (ret != glb_OK) return ret;


    /* prepare message for controller */
    buf[0] = '\0';
    sprintf(buf, "ChangeState %s 7", data->id);

    data->control_msg = strdup(buf);
    data->control_msg_size = strlen(buf);

    return glb_OK;
}


static int
glbDelivery_add_search_results(struct glbDelivery *self, 
			      struct glbData *data)
{
    char buf[GLB_TEMP_BUF_SIZE];
    char *text;
    struct glbResult *res;
    int ret;

    printf("\n\n    Delivery Register SAVE search results for \"%s\": \"%s\"\n", 
	   data->ticket, data->results);

    res = malloc(sizeof(struct glbResult));
    if (!res) return glb_NOMEM;

    /* construct result record */
    res->timestamp = time(NULL);
    res->text = strdup(data->results);
    if (!res->text) return glb_NOMEM;
    res->text_size = data->result_size;
    res->next = NULL;
 
    /* save record */
    ret = self->search_cache->set(self->search_cache, 
				  (const char*)data->ticket,
				  (void*)res);
    if (ret != glb_OK) return ret;

    /* construct reply message */
    data->reply = strdup(GLB_DELIVERY_OK);
    if (!data->reply) return glb_NOMEM;

    data->reply_size = GLB_DELIVERY_OK_SIZE;

    /* TODO: inform controller about new results */
    /*sprintf(buf, "ChangeState %s 7", data->id);
    data->control_msg = strdup(buf);
    data->control_msg_size = strlen(buf);*/

    return glb_OK;
}

static int
glbDelivery_process(struct glbDelivery *self, 
		    struct glbData *data)
{
    xmlDocPtr doc;
    xmlNodePtr root, cur_node;
    char *value;
    char *action = NULL;
    size_t action_size;

    int ret = glb_OK;

    if (!data->spec) return glb_FAIL;

    doc = xmlReadMemory(data->spec, 
			data->spec_size, 
			"none.xml", NULL, 0);
    if (!doc) {
        fprintf(stderr, "   -- Failed to parse document :(\n");
	return glb_FAIL;
    }

    printf("    ++ Delivery #%s Register: XML spec parse OK!\n", 
	   self->name);

    root = xmlDocGetRootElement(doc);
    if (!root) {
	fprintf(stderr,"empty document\n");
	ret = glb_FAIL;
	goto error;
    }

    if (xmlStrcmp(root->name, (const xmlChar *) "spec")) {
	fprintf(stderr,"Document of the wrong type: the root node " 
		" must be \"spec\"");
	ret = glb_FAIL;
	goto error;
    }

    ret = glb_copy_xmlattr(root, "action", 
			   &action, &action_size);

    if (!action) goto error;

    ret = glb_copy_xmlattr(root, "obj_id", 
			   &data->id, &data->id_size);

    ret = glb_copy_xmlattr(root, "ticket", 
			   &data->ticket, &data->ticket_size);
 
    ret = glb_copy_xmlattr(root, "topics", 
			   &data->metadata, &data->metadata_size);

    if (!strcmp(action, "add_meta")) {
	ret = glbDelivery_add_meta(self, data);
    }

    if (!strcmp(action, "get_meta")) {
	ret = glbDelivery_get_meta(self, data);
    }

    if (!strcmp(action, "add_search_results")) {

	/* parse the results */
	/*for (cur_node = root->children; cur_node; 
	     cur_node = cur_node->next) {
	    if (cur_node->type != XML_ELEMENT_NODE) continue;

	    if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"results"))) {
		value = (char*)xmlNodeGetContent(cur_node);
		if (!value) continue;

		data->results = strdup(value);
		data->result_size = strlen(value);

		xmlFree(value);
	    }
	    }*/
	if (!data->results) goto error;
	if (!data->ticket) goto error;
	
	ret = glbDelivery_add_search_results(self, data);
    }

    if (!strcmp(action, "get_search_results")) {

	printf("  prepare results for ticket %s\n", data->ticket);

	ret = glbDelivery_get_search_results(self, data);
    }

 error:


    xmlFreeDoc(doc);

    return ret;
}



/**
 *  glbDelivery network service startup
 */
static int 
glbDelivery_start(struct glbDelivery *self)
{
    void *context;
    void *service;
    void *control;

    struct glbData *data;

    const char *reply = NULL;
    size_t reply_size = 0;

    char buf[GLB_TEMP_BUF_SIZE];

    int i, ret;

    const char *err_msg = "{\"error\": \"incorrect call\"}";
    size_t err_msg_size = strlen(err_msg);

    const char *wait_msg = "{\"wait\": \"1\"}";
    size_t wait_msg_size = strlen(wait_msg);

    context = zmq_init(1);

    control = zmq_socket(context, ZMQ_PUSH);
    if (!control) return glb_FAIL;
    ret = zmq_connect(control, "tcp://127.0.0.1:5561");

    service = zmq_socket(context, ZMQ_REP);
    if (!service) return glb_FAIL;
    ret = zmq_connect(service, "tcp://127.0.0.1:6903");

    ret = glbData_new(&data);
    if (ret != glb_OK) return glb_FAIL;

    printf("\n\n    ++ %s is up and running!...\n\n",
	   self->name);

    while (1) {
	/* reset data */
	data->reset(data);
	reply = err_msg;
	reply_size = err_msg_size;

	printf("\n    ++ DELIVERY service is waiting for new tasks...\n");

        data->spec = s_recv(service, &data->spec_size);
        data->results = s_recv(service, &data->result_size);

	printf("    ++ DELIVERY service has got spec:\n"
               "       %s results: %s\n", data->spec, data->results);

	ret = self->process(self, data);
	if (ret == glb_NEED_WAIT) {
	    reply = wait_msg;
	    reply_size = wait_msg_size;
	    goto final;
	}

	/* notify controller */
	/*if (data->control_msg) {
		   }*/

	if (data->reply_size) {
	    reply = (const char*)data->reply;
	    reply_size = data->reply_size;
	}

    final:

	printf("    == Delivery reply:\n   %s\n", reply);

	s_send(service, reply, reply_size);

        fflush(stdout);
    }

    /* we never get here */
    zmq_close(service);
    zmq_term(context);

    return glb_OK;
}


static int
glbDelivery_init(struct glbDelivery *self)
{
    self->str = glbDelivery_str;
    self->del = glbDelivery_del;

    self->start = glbDelivery_start;
    self->process = glbDelivery_process;

    return glb_OK;
}

int glbDelivery_new(struct glbDelivery **rec,
		   const char *config)
{
    struct glbDelivery *self;
    struct glbPartition *part;
    struct glbMaze *maze;

    struct ooDict *dict, *delivery;
    char buf[GLB_TEMP_BUF_SIZE];
    char *path;
    int i, ret = glb_OK;
    
    self = malloc(sizeof(struct glbDelivery));
    if (!self) return glb_NOMEM;

    memset(self, 0, sizeof(struct glbDelivery));

    ret = ooDict_new(&self->obj_index, GLB_LARGE_DICT_SIZE);
    if (ret != oo_OK) goto error;

    ret = ooDict_new(&self->search_cache, GLB_MEDIUM_DICT_SIZE);
    if (ret != oo_OK) goto error;


    glbDelivery_init(self); 


    *rec = self;

    return glb_OK;

 error:

    glbDelivery_del(self);

    return ret;
}
