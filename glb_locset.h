#ifndef GLB_LOCSET_H
#define GLB_LOCSET_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "glb_config.h"


typedef enum glb_loc_type { GLB_LOC_NUMERIC,
			    GLB_LOC_ID, 
			    GLB_LOC_NAME } glb_loc_type;

typedef enum glb_loc_rank { GLB_LOC_ROOT,
			    GLB_LOC_NODE, 
			    GLB_LOC_LEAF } glb_loc_rank;
/**
 *  Set of nested location references
 *    1.1
 *    2.1.2
 *    2.3.1 etc.
 */

typedef struct glbLocSet
{
    char *name;
    glb_loc_type type;
    glb_loc_rank rank;

    struct glbLocSet *parent;

    struct glbLocSet **children;
    size_t num_children;
    
    char *obj_state_id;

    /* serialized bytecode */
    char *bytecode;
    size_t bytecode_size;

    char *bytecode_idx;
    size_t bytecode_idx_size;

    /***********  public methods  ***********/
    int (*init)(struct glbLocSet *self);
    int (*del)(struct glbLocSet  *self);
    char* (*str)(struct glbLocSet  *self, size_t depth);

    int (*import)(struct glbLocSet *self, 
		  const char *input);

    int (*pack)(struct glbLocSet *self);
    int (*unpack)(struct glbLocSet *self);

    int (*list)(struct glbLocSet *self);
    int (*lookup)(struct glbLocSet *self);

} glbLocSet;


extern int glbLocSet_new(struct glbLocSet **self);

#endif
