#ifndef GLB_SET_H
#define GLB_SET_H

#include <stdio.h>
#include <stdlib.h>

#include "glb_config.h"

struct glbSet
{
    const char *name;
    size_t name_size;

    const char *path;
    size_t path_size;

    /* number of ids in set */
    size_t num_objs;

    struct glbSetFile *data; /* data on hard drive */
    struct glbIndexTree *index; /* data index */

    /* for active usage lists */
    struct glbSet *next;
    struct glbSet *prev;

    /**********  public methods  **********/

    int (*init)(struct glbSet *self, 
		const char *path,
		const char *name);

    int (*del)(struct glbSet *self); /* destructor */

    /* add id to this set. bytecode contains locset */
    int (*add)(struct glbSet *self, const char *id);
    int (*lookup)(struct glbSet *self, const char *id);

} glbSet;

/* constructor */
extern int glbSet_new(struct glbSet **self);

#endif
