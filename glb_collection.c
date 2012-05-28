#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <pthread.h>

#include <zmq.h>

#include "glb_config.h"
#include "glb_collection.h"


void *glbColl_add_storage(void *arg)
{
    void *context;
    void *frontend;
    void *backend;
    int ret;

    context = zmq_init(1);

    frontend = zmq_socket(context, ZMQ_SUB);
    if (!frontend) pthread_exit(NULL);
    zmq_bind(frontend, "tcp://127.0.0.1:6910");
    zmq_setsockopt(frontend, ZMQ_SUBSCRIBE, "", 0);

    backend = zmq_socket(context, ZMQ_PUB);
    if (!backend) pthread_exit(NULL);
    zmq_bind(backend, "tcp://127.0.0.1:6911");

    printf("   Collection storage device ready....\n");

    zmq_device(ZMQ_FORWARDER, frontend, backend);

    /* we never get here */
    zmq_close(frontend);
    zmq_close(backend);
    zmq_term(context);

    return;
}


static int
glbColl_str(struct glbColl *self)
{
    printf("<struct glbColl at %p>\n", self);


    return glb_OK;
}

static int
glbColl_del(struct glbColl *self)
{
    free(self);
    return glb_OK;
}

 
static int 
glbColl_find_route(struct glbColl *self,
		   const char *spec,
		   const char **dest_coll_addr)
{
    const char *dest = NULL;
    
 
    printf("    finding route to the appropriate Collection...\n");

    *dest_coll_addr = dest;

    return glb_OK;
}



/**
 *  glbCollection network service startup
 */
static int 
glbColl_start(struct glbColl *self)
{
    void *context;
    void *frontend;
    void *backend;
    pthread_t storage_outlet;
    int ret;

    ret = pthread_create(&storage_outlet, 
			 NULL, 
			 glbColl_add_storage, (void*)self);

    context = zmq_init(1);

    frontend = zmq_socket(context, ZMQ_PULL);
    backend = zmq_socket(context, ZMQ_PUSH);

    zmq_bind(frontend, "tcp://127.0.0.1:6908");
    zmq_bind(backend, "tcp://127.0.0.1:6909");

    printf("  ++ Collection server is up and running!...\n\n");

    zmq_device(ZMQ_QUEUE, frontend, backend);

    /* we never get here */
    zmq_close(frontend);
    zmq_close(backend);
    zmq_term(context);

    return glb_OK;
}


static int
glbColl_init(struct glbColl *self)
{
    self->str = glbColl_str;
    self->del = glbColl_del;
    self->start = glbColl_start;
    self->find_route = glbColl_find_route;

    return glb_OK;
}

int glbColl_new(struct glbColl **rec,
		const char *config)
{
    struct glbColl *self;
    struct ooDict *dict, *storage;
    int result;
    size_t i, count; /* max num of docs */
    
    self = malloc(sizeof(struct glbColl));
    if (!self) return glb_NOMEM;

    glbColl_init(self);


    printf("  reading config %s...\n", config);

    *rec = self;
    return glb_OK;
}
