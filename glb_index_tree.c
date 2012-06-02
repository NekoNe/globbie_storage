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
    if (tmp->right) tmp->right->parent = tmp; 

    oldtop->right = newtop->left;
    if (oldtop->right) oldtop->right->parent = oldtop; 
    
    newtop->left = oldtop;
    if (newtop->left) newtop->left->parent = newtop;
    
    return glb_OK;
}

static int 
glbIndexTree_rotate_root(struct glbIndexTree *self, struct glbIndexTreeNode *tmp)
{
    self->root = tmp->right;
    self->root->parent = NULL; 

    tmp->right = self->root->left;
    if (tmp->right) tmp->right->parent = tmp; 

    self->root->left  = tmp;
    if (self->root->left)self->root->left->parent = self->root; 
    
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

    return glb_OK;
}

static int 
glbIndexTree_lookup(struct glbIndexTree *self, 
		    const char *id, 
		    size_t *offset, 
		    struct glbIndexTreeNode *tmp)
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
glbIndexTree_addElem(struct glbIndexTree *self, 
		     const char *id, 
		     size_t offset)
{
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
    cur->id_last = NULL;
    cur->right = NULL;
    cur->left = NULL;
    cur->offset = offset;
    cur->parent = last; /* set parent */
    if (last) last->right = cur;
    self->node_count++;
    if (!self->root) self->root = cur;
    return glb_OK;
}

static int
glbIndexTree_del(struct glbIndexTree *self)
{
    if (!self) return glb_OK;

    if (!self->array) return glb_OK;
    
    free(self->array);
    free(self);

    return glb_OK;
}

static int
glbIndexTree_init(struct glbIndexTree *self)
{
    self->last = self->array;
    self->array_offset = 0;
    self->root = NULL;
     
    self->node_count = 0;
    self->max_height = 0;

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
    size_t i;
    int result;
    
    result = glb_NOMEM;

    self = malloc(sizeof(glbIndexTree));
    if (!self) return glb_NOMEM;

    self->array = NULL;

    self->array = malloc((GLB_ID_MAX_COUNT / GLB_LEAF_SIZE) *\
			 sizeof(struct glbIndexTreeNode));
    if (!self->array) goto error;

    for (i = 0; i < GLB_ID_MAX_COUNT / GLB_LEAF_SIZE; i++) {
        self->array[i].left = NULL;
        self->array[i].right = NULL;
        self->array[i].parent = NULL;
        self->array[i].id = NULL;
        self->array[i].id_last = NULL;
        self->array[i].offset = 0;
    }

    result = glbIndexTree_init(self);
    *rec = self;
    if (result) goto error;

    return glb_OK;

error:
    glbIndexTree_del(self);
    return result;
}

