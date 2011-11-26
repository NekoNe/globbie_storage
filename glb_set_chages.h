
#include <stdio.h>
#include <stdlib.h>

#include "glb_config.h"
#include "glb_set_elem.h"

typedef struct glbSetChages
{
    struct glbSetChages *tree;

    /**********  public methods  **********/

    int (*init)(struct glbSetChanges *self);
    int (*del)(struct glbSetChanges *self);

    int (*lookup)(struct glbSetChanges *self, const char *id, struct glbSetElem *result);
    int (*update)(struct glbSetChanges *self);

} glbSetChanges;

extern int glbSetChanges_new(struct glbSetChanges **self);

