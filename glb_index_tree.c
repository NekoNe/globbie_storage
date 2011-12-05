#include <stdio.h>
#include <stdlib.h>

#include "glb_config.h"
#include "glb_index_tree.h"
#include "glb_utils.h"

size_t max(size_t a, size_t b)
{
    return a > b ? a : b;
}

static size_t 
glbIndexTree_height(struct glbIndexTree *self, struct glbIndexTreeNode *tmp)
{
    if (!tmp) return 0;
    return (size_t)1 + max(self->height(self, tmp->left), self->height(self, tmp->right));
}

static int 
glbIndexTree_rotate(struct glbIndexTree *self, struct glbIndexTreeNode *tmp)
{
    struct glbIndexTreeNode *oldtop;
    struct glbIndexTreeNode *newtop;
    
    /*TODO: only one variable can be used */
    
    oldtop = tmp->right;
    newtop = oldtop->right;
    
    tmp->right = tmp->right->right;
    oldtop->right = newtop->left;
    newtop->left = oldtop;
    return glb_OK;
}

static int 
glbIndexTree_rotate_root(struct glbIndexTree *self, struct glbIndexTreeNode *tmp)
{
    self->root = tmp->right;
    tmp->right = self->root->left;
    self->root->left  = tmp;
    
    return glb_OK;
}

static int
glbIndexTree_update(struct glbIndexTree *self, struct glbIndexTreeNode *tmp)
{
    size_t difference;
    int result;

    if (!tmp) return glb_IDXT_NTD;
    
    result = self->update(self, tmp->right);
    
    if (result == glb_IDXT_NEED_ROTATE) 
        self->rotate(self, tmp);
        
    difference = self->height(self, tmp->right) - self->height(self, tmp->left);
    
    if (difference != 2) return glb_IDXT_NTD; /* nothing to do */

    if (tmp != self->root) return glb_IDXT_NEED_ROTATE; /* I don't know how to name 2. it will be 2 always */
    self->rotate_root(self, tmp);     
    
}

static int 
glbIndexTree_lookup(struct glbIndexTree *self, const char *id, size_t *offset, struct glbIndexTreeNode *tmp)
{
    int cmp_res;
    int lookup_res;

    if (!tmp) return glb_NO_RESULTS;
  
    cmp_res = compare(tmp->id, id);

    if (cmp_res == glb_LESS) {
        lookup_res = self->lookup(self, id, offset, tmp->right);
       
        if (lookup_res == glb_OK) {
            return glb_OK;
        }

        if (lookup_res == glb_NO_RESULTS) {
            *offset = tmp->offset;
            return glb_OK;
        }
    }
    
    if (cmp_res == glb_MORE) {
        lookup_res = self->lookup(self, id, offset, tmp->left);
        return lookup_res; 
    }

   *offset = tmp->offset;
   return glb_OK; 
}

static int
glbIndexTree_addElem(struct glbIndexTree *self, const char *id, size_t offset)
{
   /* struct glbIndexTreeNode **tmp;
    struct glbIndexTreeNode **last;
    
    tmp = &(self->root);
    last = NULL;
    while (*tmp) {
        last = tmp;
        tmp = &(*tmp)->right;    
    }
    *tmp = self->last++;
    (*tmp)->id = id;
    (*tmp)->right = NULL;
    (*tmp)->left = NULL;
    (*tmp)->offset = offset;
    self->node_count++;
    if (last) (*last)->right = *tmp;  
     */


    struct glbIndexTreeNode *cur;
    struct glbIndexTreeNode *last;

    cur = self->root;
    last = NULL;
   
    while (cur) {
        last = cur;
        cur = cur->right;
    }

    cur = self->last++;
    cur->id = id;
    cur->right = NULL;
    cur->left = NULL;
    cur->offset = offset;
    cur->parent = last;
    self->node_count++;
    if (!self->root) self->root = cur;
    return glb_OK;
}

static int
glbIndexTree_del(struct glbIndexTree *self)
{
    /*TODO*/
    return glb_OK;
}

static int
glbIndexTree_init(struct glbIndexTree *self)
{
    /* TODO: do not allocate all memory at a time and check allocation was ok */
    self->array = malloc((GLB_RADIX_BASE * GLB_RADIX_BASE * GLB_RADIX_BASE / GLB_LEAF_SIZE) * sizeof(struct glbIndexTreeNode));
    self->last = self->array;
    self->array_offset = 0;
    self->root = NULL;
    
    self->node_count = 0;

    self->init = glbIndexTree_init;
    self->del = glbIndexTree_del;

    self->addElem = glbIndexTree_addElem;
    self->lookup = glbIndexTree_lookup;
    self->update = glbIndexTree_update;
    self->rotate_root = glbIndexTree_rotate_root;
    self->rotate = glbIndexTree_rotate;
    self->height = glbIndexTree_height;

    return glb_OK;
}


int glbIndexTree_new(struct glbIndexTree **rec)
{
    struct glbIndexTree *self;

    self = malloc(sizeof(glbIndexTree));
    if (!self) return glb_NOMEM;
    
    glbIndexTree_init(self);

    *rec = self;

    return glb_OK;
}
