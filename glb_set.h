
#include <stdio.h>
#include <stdlib.h>

#include "glb_config.h"
/*#include "glb_set_elem.h"*/
#include "glb_set_file.h"
#include "glb_index_tree.h"
/*#include "glb_set_tree_of_changes.h"*/

typedef struct glbSet
{
    char *name;
    char *env_path;

    size_t index_counter;

    struct glbSetFile *data; /* data on hard drive */
    char *buffered_data; /* same data in RAM (not copy!) */
    struct glbIndexTree *index;

    /**********  public methods  **********/

    int (*init)(struct glbSet *self);
    int (*del)(struct glbSet *self);

    int (*add)(struct glbSet *self, const char *id, const char *bytecode, size_t bytecode_size);
    int (*lookup)(struct glbSet *self, const char *id, size_t *locset_pointer);

    int (*intersect)(struct glbSet *self, struct glbSet *another, struct glbSet *result);

} glbSet;

extern int glbSet_new(struct glbSet **self);

