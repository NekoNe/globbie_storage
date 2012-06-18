#ifndef GLB_SET_H
#define GLB_SET_H

#include <stdio.h>
#include <stdlib.h>

#include "glb_config.h"

struct glbSet;

struct glbSetRef
{
    const char *name;
    struct glbSet *set;

    size_t num_items;

    struct glbSetRef *next;
};

struct glbSet
{
    const char *name;
    size_t name_size;

    const char *path;
    size_t path_size;

    /* number of ids in set */
    size_t num_objs;

    struct glbSetFile *data;    /* data on hard drive */
    struct glbIndexTree *index; /* data index */

    /* logical group */
    struct glbSetRef *refs;
    size_t num_refs;

    /* for active usage lists */
    struct glbSet *next;
    struct glbSet *prev;

    /**********  public methods  **********/

    int (*init)(struct glbSet *self, 
		const char *path,
		const char *name,
		size_t name_size);

    int (*del)(struct glbSet *self);

    /* add id to this set. bytecode contains locset */
    int (*add)(struct glbSet *self, const char *id);
    int (*lookup)(struct glbSet *self, const char *id);

} glbSet;

/* constructor */
extern int glbSet_new(struct glbSet **self);

#endif
