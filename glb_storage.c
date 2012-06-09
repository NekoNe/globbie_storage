#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <zmq.h>

#include "oodict.h"
#include "glb_set.h"
#include "glb_index_tree.h"
#include "glb_set_file.h"
#include "glb_request_handler.h"
#include "glb_config.h"
#include "glb_interpreter.h"

#include "glb_storage.h"
#include "glb_partition.h"
#include "glb_maze.h"
#include "glb_utils.h"

#include "zhelpers.h"

static int glbStorage_add_meta(struct glbStorage *self,
			       struct glbData *data);

static int glbStorage_add_search_results(struct glbStorage *self,
			       struct glbData *data);

static int
glbStorage_str(struct glbStorage *self)
{
    printf("<struct glbStorage at %p>\n", self);


    return glb_OK;
}


static int
glbStorage_del(struct glbStorage *self)
{
    struct glbMaze *maze;
    int i;
    if (GLB_COLLECTION_DEBUG_LEVEL_1)
        printf("deleting struct glbStorage at %p...\n", self);

    if (!self) return glb_OK;
    
    if (self->num_agents) {
	for (i = 0; i  < self->num_agents; i++) {
	    maze = self->mazes[i];
	    if (!maze) continue;
	    maze->del(maze);
	}
	free(self->mazes);
    }


    free(self);
    return glb_OK;
}


static int
glbStorage_get_meta(struct glbStorage *self, 
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
glbStorage_get_search_results(struct glbStorage *self, 
			      struct glbData *data)
{
    char *results;
    const char *tag_begin = "{\"results\": \"";

    const char *tag_close = "\"}";
    size_t result_size;

    char *buf;
    unsigned char *c;
    int i, ret;

    if (!data->ticket) return glb_FAIL;

    results = (char*)self->search_cache->get(self->search_cache, 
					     (const char*)data->ticket);

    printf("  RESULTS in Dict: %s\n", results);

    if (!results) {
	return glb_NEED_WAIT;
    }

    /* remove/escape symbols */
    /*c = (unsigned char*)results;
    while (*c) {
	if (*c < 32) {
	    *c = ' ';
	}
	if (*c == '\"') *c = ' ';
	if (*c == '\'') *c = ' ';
	if (*c == '&') *c = ' ';
	if (*c == '\\') *c = ' '; 
	c++;
	}*/

    result_size = strlen(results);

    data->reply_size = strlen(tag_begin) +\
	result_size +  strlen(tag_close);

    data->reply = malloc(data->reply_size + 1);
    if (!data->reply) return glb_FAIL;

    buf = data->reply;
    memcpy(buf, tag_begin, strlen(tag_begin));
    buf += strlen(tag_begin);

    memcpy(buf, results, result_size);
    buf += result_size;

    memcpy(buf, tag_close, strlen(tag_close));
    buf += strlen(tag_close);

    *buf = '\0';

    return glb_OK;
}



static int
glbStorage_add_meta(struct glbStorage *self, 
		    struct glbData *data)
{
    char buf[GLB_TEMP_BUF_SIZE];
    int ret;

    printf("\n\n    Storage Register ADD metadata for \"%s\": \"%s\"\n", 
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
glbStorage_add_search_results(struct glbStorage *self, 
			      struct glbData *data)
{
    char buf[GLB_TEMP_BUF_SIZE];
    int ret;


    printf("\n\n    Storage Register SAVE search results for \"%s\": \"%s\"\n", 
	   data->ticket, data->results);

    char *results;
    results = strdup(data->results);

    ret = self->search_cache->set(self->search_cache, 
				  (const char*)data->ticket,
				  (void*)results);
    if (ret != glb_OK) return ret;



    /* TODO: inform controller about new results */
    /*sprintf(buf, "ChangeState %s 7", data->id);
    data->control_msg = strdup(buf);
    data->control_msg_size = strlen(buf);*/

    return glb_OK;
}

static int
glbStorage_process(struct glbStorage *self, 
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

    printf("    ++ Storage #%s Register: XML spec parse OK!\n", 
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
	ret = glbStorage_add_meta(self, data);
    }

    if (!strcmp(action, "get_meta")) {
	ret = glbStorage_get_meta(self, data);
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
	
	ret = glbStorage_add_search_results(self, data);
    }

    if (!strcmp(action, "get_search_results")) {

	printf("  prepare results for ticket %s\n", data->ticket);

	ret = glbStorage_get_search_results(self, data);
    }

 error:


    xmlFreeDoc(doc);

    return ret;
}



/**
 *  glbStorage network service startup
 */
static int 
glbStorage_start(struct glbStorage *self)
{
    void *frontend;
    void *backend;

    int i, ret;

    /* create queue device */
    frontend = zmq_socket(self->context, ZMQ_PULL);
    if (!frontend) return glb_FAIL;

    backend = zmq_socket(self->context, ZMQ_PUSH);
    if (!backend) return glb_FAIL;

    zmq_bind(frontend, "inproc://inbox");
    zmq_bind(backend, "inproc://outbox");

    printf("\n\n    ++ %s STORAGE SERVER is up and running!...\n\n",
	   self->name);

    zmq_device(ZMQ_QUEUE, frontend, backend);

    /* we never get here */
    zmq_close(frontend);
    zmq_close(backend);
    zmq_term(self->context);

    return glb_OK;
}

static int
glbStorage_init(struct glbStorage *self)
{
    self->str = glbStorage_str;
    self->del = glbStorage_del;

    self->start = glbStorage_start;
    self->process = glbStorage_process;

    return glb_OK;
}

int glbStorage_new(struct glbStorage **rec,
		   const char *config)
{
    struct glbStorage *self;
    struct glbPartition *part;
    struct glbMaze *maze;

    struct ooDict *dict, *storage;
    char buf[GLB_TEMP_BUF_SIZE];
    char *path;
    int i, ret = glb_OK;
    
    self = malloc(sizeof(struct glbStorage));
    if (!self) return glb_NOMEM;

    memset(self, 0, sizeof(struct glbStorage));

    /* TODO: read path from config */
    self->path = strdup("storage");

    ret = ooDict_new(&self->obj_index, GLB_LARGE_DICT_SIZE);
    if (ret != oo_OK) goto error;

    ret = ooDict_new(&self->search_cache, GLB_MEDIUM_DICT_SIZE);
    if (ret != oo_OK) goto error;

    self->partitions = malloc(sizeof(struct glbPartition*) * GLB_NUM_AGENTS);
    if (!self->partitions) {
        ret = glb_NOMEM;
	goto error;
    }

    self->mazes = malloc(sizeof(struct glbMaze*) * GLB_NUM_AGENTS);
    if (!self->mazes) {
        ret = glb_NOMEM;
	goto error;
    }

    for (i = 0; i < GLB_NUM_AGENTS; i++) {

	/* partition */
	ret = glbPartition_new(&part);
	if (ret) goto error;

	part->id = i;
	part->env_path = self->path;

	ret = part->init(part);
	if (ret != glb_OK) goto error;

	self->partitions[i] = part;


	/* maze */
	ret = glbMaze_new(&maze);
	if (ret) goto error;

	maze->id = i;
	maze->storage_path = part->path;

	buf[0] = '\0';
	sprintf(buf, "maze%d", i);

	path = strdup(buf);
	if (!path) {
	    ret = glb_NOMEM;
	    goto error;
	}
	maze->path = path;
	maze->path_size = strlen(path);
	self->mazes[i] = maze;

	/* TODO: load existing indices into memory */

    }

    self->num_agents = GLB_NUM_AGENTS;

    glbStorage_init(self); 


    *rec = self;

    return glb_OK;

 error:

    glbStorage_del(self);

    return ret;
}
