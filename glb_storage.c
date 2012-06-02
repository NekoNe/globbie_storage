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

static int glbStorage_add(struct glbStorage *self,
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
glbStorage_get(struct glbStorage *self, 
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
 
    ret = glb_copy_xmlattr(root, "topics", 
			   &data->metadata, &data->metadata_size);

    if (!strcmp(action, "add")) {
	ret = glbStorage_add(self, data);
    }

    if (!strcmp(action, "get")) {
	ret = glbStorage_get(self, data);
    }

 error:


    xmlFreeDoc(doc);

    return ret;
}


static int
glbStorage_add(struct glbStorage *self, 
		   struct glbData *data)
{
    char buf[GLB_TEMP_BUF_SIZE];
    int ret;

    printf("\n\n    Storage Register ADD obj_id: \"%s\" topics: \"%s\"\n", 
	   data->id, data->metadata);

    ret = self->obj_index->set(self->obj_index, 
			       (const char*)data->id, 
			       (void*)strdup(data->metadata));
    if (ret != glb_OK) return ret;

    /* prepare message for controller */

    sprintf(buf, "ChangeState %s 7", data->id);
    data->control_msg = strdup(buf);
    data->control_msg_size = strlen(buf);

    return glb_OK;
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
    self->add = glbStorage_add;
    self->get = glbStorage_get;

    return glb_OK;
}

int glbStorage_new(struct glbStorage **rec,
		   const char *config)
{
    struct glbStorage *self;
    struct ooDict *dict, *storage;
    char buf[GLB_TEMP_BUF_SIZE];
    char *path;
    int i, ret;
    
    self = malloc(sizeof(struct glbStorage));
    if (!self) return glb_NOMEM;

    memset(self, 0, sizeof(struct glbStorage));

    ret = ooDict_new(&self->obj_index, GLB_LARGE_DICT_SIZE);
    if (ret != oo_OK) return ret;

    self->partitions = malloc(sizeof(struct glbPartition*) * GLB_NUM_AGENTS);
    if (!self->partitions) {
        glbStorage_del(self);
        return glb_NOMEM;
    }

    self->mazes = malloc(sizeof(struct glbMaze*) * GLB_NUM_AGENTS);
    if (!self->mazes) {
        glbStorage_del(self);
        return glb_NOMEM;
    }

    for (i = 0; i < GLB_NUM_AGENTS; i++) {
	ret = glbPartition_new(&self->partitions[i]);
	if (ret) {
	    glbStorage_del(self);
	    return ret;
	}
	self->partitions[i]->id = i;
	    

	ret = glbMaze_new(&self->mazes[i]);
	if (ret) {
	    glbStorage_del(self);
	    return ret;
	}

	self->mazes[i]->id = i;

	sprintf(buf, "maze%d", i);
	path = strdup(buf);
	if (!path) return glb_NOMEM;

	self->mazes[i]->path = path;
	self->mazes[i]->path_size = strlen(path);

    }

    self->num_agents = GLB_NUM_AGENTS;

    glbStorage_init(self); 


    *rec = self;

    return glb_OK;
}
