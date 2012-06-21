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
    const char *obj_ids;
    size_t num_obj_ids;
    size_t obj_ids_size;

    struct glbIndexTree *index; /* data index */

    struct glbSetFile *data;    /* data on hard drive */

    /* logical group */
    struct glbSet *refs;
    size_t num_refs;

    /* for lists */
    struct glbSet *next;
    struct glbSet *prev;

    struct glbSet *next_ref;

    /**********  public methods  **********/

    int (*init)(struct glbSet *self);

    int (*del)(struct glbSet *self);
    int (*str)(struct glbSet *self);

    /* add id to this set. bytecode contains locset */
    int (*add)(struct glbSet *self, const char *id);
    int (*lookup)(struct glbSet *self, const char *id);
    int (*build_index)(struct glbSet *self);
    int (*read_buf)(struct glbSet *self, 
		    size_t offset, 
		    char *buffer, 
		    size_t size, 
		    size_t *result);

} glbSet;

/* constructor */
extern int glbSet_new(struct glbSet **self);

#endif
