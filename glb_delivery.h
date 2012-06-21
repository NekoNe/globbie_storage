#ifndef GLB_DELIVERY_H
#define GLB_DELIVERY_H

#define GLB_NUM_AGENTS 1

#include <time.h>

#include "glb_utils.h"
#include "oodict.h"

struct glbResult
{
    char *text;
    size_t text_size;
    time_t timestamp;
    struct glbResult *next;
};


struct glbDelivery
{
    char *name;
    char *path;
    char *service_address;


    struct ooDict *obj_index;

    struct ooDict *search_cache;

    /**********  interface methods  **********/
    int (*del)(struct glbDelivery *self);
    int (*str)(struct glbDelivery *self);

    int (*start)(struct glbDelivery *self);

    int (*process)(struct glbDelivery *self, 
		   struct glbData *data);

    int (*add)(struct glbDelivery *self, 
		   struct glbData *data);

    int (*get)(struct glbDelivery *self, 
		   struct glbData *data);

};

extern int glbDelivery_new(struct glbDelivery **self, 
			  const char *config);
#endif
