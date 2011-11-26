#include <stdio.h>
#include <stdlib.h>

#include "glb_config.h"

typedef struct glbIndexTreeNode
{
    struct glbIndexTreeNode *left;
    struct glbIndexTreeNode *right;
    
    char *id;
    size_t offset;
    
    
};