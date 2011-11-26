
#include <stdio.h>
#include <stdlib.h>

#include "glb_set_elem.h"

typedef struct glbSetIndex
{
    char *path;

    /**********  public methods  **********/

    int (*init)(struct glbSetIndex *self);
    int (*del)(struct glbSetIndex *self);

    int (*hash)(struct glbSetIndex *self, char *id, size_t *offset);
    int (*add)(struct glbSetIndex *self, struct glbSetElem *elem);
    int (*update)(struct glbSetIndex *self);
    
} glbSetIndex;

extern int glbSetIndex_new(struct glbSetIndex **self);

