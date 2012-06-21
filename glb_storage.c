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

    /*ret = ooDict_new(&self->obj_index, GLB_LARGE_DICT_SIZE);
    if (ret != oo_OK) goto error;

    ret = ooDict_new(&self->search_cache, GLB_MEDIUM_DICT_SIZE);
    if (ret != oo_OK) goto error;*/

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
	maze->partition = part;

	self->mazes[i] = maze;

	/* TODO: load existing indices into memory */
	maze->init(maze);


    }

    self->num_agents = GLB_NUM_AGENTS;

    glbStorage_init(self); 


    *rec = self;

    return glb_OK;

 error:

    glbStorage_del(self);

    return ret;
}
