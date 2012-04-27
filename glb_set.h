#include <stdio.h>
#include <stdlib.h>

#include "glb_config.h"

typedef struct glbSet
{
    char name[GLB_NAME_LENGTH];
    char env_path[GLB_NAME_LENGTH];

    /* number of ids in set */
    size_t index_counter;

    struct glbSetFile *data; /* data on hard drive */
    struct glbIndexTree *index; /* data index */

    /**********  public methods  **********/

    int (*init)(struct glbSet *self, const char *name, const char *env_path);
    int (*del)(struct glbSet *self); /* destructor */

    /* add id to this set. bytecode contains locset */
    int (*add)(struct glbSet *self, const char *id, const char *bytecode, size_t bytecode_size);
    int (*lookup)(struct glbSet *self, const char *id, size_t *locset_pointer);

} glbSet;

/* constructor */
extern int glbSet_new(struct glbSet **self, const char *name, const char *env_path);

