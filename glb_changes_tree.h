
#include <stdio.h>
#include <stdlib.h>

#include "glb_config.h"


typedef struct glbChagesTree
{
    char *buffer;
    size_t buffer_size;
    size_t buffer_end;

    /**********  public methods  **********/

    int (*init)(struct glbChangesTree *self);
    int (*del)(struct glbChangesTree *self);
    
    /**********  internal methods **********/

    int (*lookup)(struct glbChangesTree *self, const char *id, size_t *offset, size_t *pointer_offset);
    int (*changeState)(struct glbChangesTree *self, size_t offset, int status);
    int (*write)(struct glbChangesTree *self, struct glbSetElem *elem, int status, size_t pointer_offset);

    /**********  interfaces  **********/
    
    int (*update)(struct glbChangesTree *self);
    int (*addElem)(struct glbChangesTree *self, struct glbSetElem *elem);     
    int (*delElem)(struct glbChangesTree *self, struct glbSetElem *elem);     
    int (*changeElem)(struct glbChangesTree *self, struct glbSetElem *elem);     
    int (*lookupElem)(struct glbChangesTree *self, const char *id, struct glbSetElem *result);
}
