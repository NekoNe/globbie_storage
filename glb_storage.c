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

#include "zhelpers.h"


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

    printf("starting...\n");

    /* create queue device */
    frontend = zmq_socket(self->context, ZMQ_PULL);
    if (!frontend) return glb_FAIL;

    backend = zmq_socket(self->context, ZMQ_PUSH);
    if (!backend) return glb_FAIL;

    zmq_bind(frontend, "inproc://inbox");
    zmq_bind(backend, "inproc://outbox");

    printf("  ++ Storage server is up and running!...\n\n");

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
    struct ooDict *dict, *storage;
    int ret;
    size_t i, count; /* max num of docs */
    
    self = malloc(sizeof(struct glbStorage));
    if (!self) return glb_NOMEM;

    memset(self, 0, sizeof(struct glbStorage));

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
    }

    self->num_agents = GLB_NUM_AGENTS;

    glbStorage_init(self); 


    *rec = self;

    return glb_OK;
}
