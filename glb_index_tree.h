#include <stdio.h>
#include <stdlib.h>

#include "glb_config.h"
#include "glb_utils.h"

typedef struct glbIndexTreeNode
{
    struct glbIndexTreeNode *left;
    struct glbIndexTreeNode *right;
    struct glbIndexTreeNode *parent;

    const char *id;
    size_t offset;
    const char *id_last;    
    
} glbIndexTreeNode;

typedef struct glbIndexTree
{
    size_t node_count;
    size_t max_height; /* height of binary tree */
    size_t array_offset;

    struct glbIndexTreeNode *array;
    struct glbIndexTreeNode *root;
    struct glbIndexTreeNode *last;
    
    /********** public methods   **********/
    int (*init)(struct glbIndexTree *self);
    /* destructor */
    int (*del)(struct glbIndexTree *self);

    /* find id */
    int (*addElem)(struct glbIndexTree *self, const char *id, size_t offset);

    /* lookup leaf containig id */
    int (*lookup)(struct glbIndexTree *self, 
		  const char *id, 
		  size_t *offset, 
		  struct glbIndexTreeNode *tmp);

    /* tree balance */
    int (*update)(struct glbIndexTree *self, struct glbIndexTreeNode *tmp);
    /* tree height */
    size_t (*height)(struct glbIndexTree *self, struct glbIndexTreeNode *tmp); 
    /* balancing functions */
    int (*rotate)(struct glbIndexTree *self, struct glbIndexTreeNode *tmp); 
    int (*rotate_root)(struct glbIndexTree *self, struct glbIndexTreeNode *tmp); 

} glbIndexTree;

/* constructor */
extern int glbIndexTree_new(struct glbIndexTree **self);
