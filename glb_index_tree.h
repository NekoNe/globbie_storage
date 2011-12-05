#include <stdio.h>
#include <stdlib.h>


#include "glb_config.h"
#include "glb_utils.h"

typedef struct glbIndexTreeNode
{
    struct glbIndexTreeNode *left;
    struct glbIndexTreeNode *right;
    struct glbIndexTreeNode *parent;

    char *id;
    size_t offset;
    char *id_last;    
    
} glbIndexTreeNode;

typedef struct glbIndexTree
{
    size_t leaf_size;
    size_t node_count;
    size_t max_height;
    size_t array_offset;
    /*size_t height;*/

    struct glbIndexTreeNode *array;
    struct glbIndexTreeNode *root;
    struct glbIndexTreeNode *last;
    
    /********** public methods   **********/
    int (*init)(struct glbIndexTree *self);
    int (*del)(struct glbIndexTree *self);

    int (*addElem)(struct glbIndexTree *self, const char *id, size_t offset);
    int (*lookup)(struct glbIndexTree *self, const char *id, size_t *offset, struct glbIndexTreeNode *tmp);

    int (*update)(struct glbIndexTree *self, struct glbIndexTreeNode *tmp);
    size_t (*height)(struct glbIndexTree *self, struct glbIndexTreeNode *tmp); 
    int (*rotate)(struct glbIndexTree *self, struct glbIndexTreeNode *tmp); 
    int (*rotate_root)(struct glbIndexTree *self, struct glbIndexTreeNode *tmp); 

} glbIndexTree;



extern int glbIndexTree_new(struct glbIndexTree **self);
