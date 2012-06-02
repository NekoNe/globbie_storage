
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glb_config.h"
#include "glb_set.h"
#include "glb_set_file.h"
#include "glb_index_tree.h"

#define GLB_DEBUG_SET_LEVEL_1 1
#define GLB_DEBUG_SET_LEVEL_2 1
#define GLB_DEBUG_SET_LEVEL_3 1
#define GLB_DEBUG_SET_LEVEL_4 1

static int 
glbSet_lookup(struct glbSet *self, const char *id)
{
    size_t offset;
    int ret;

    if (GLB_DEBUG_SET_LEVEL_3) 
        printf("Looking up id \"%s\" in the index...\n", id);

    ret = self->index->lookup(self->index, id, &offset, self->index->root); 
    if (ret == glb_NO_RESULTS) return ret;

    if (GLB_DEBUG_SET_LEVEL_4) 
        printf("Reading offset \"%d\" from the data file...\n", offset);
   
    ret = self->data->lookup(self->data, id, offset);
    if (ret == glb_NO_RESULTS) return ret;

    if (GLB_DEBUG_SET_LEVEL_3)
        printf("Lookup success!\n");

    return glb_OK;
}

static int
glbSet_add(struct glbSet *self, const char *id)
{
    int ret;
    size_t offset;
    struct glbIndexTreeNode *last;
    
    /* saving to file */
    /*ret = self->data->addByteCode(self->data, bytecode, bytecode_size, &offset);*/

    if (GLB_DEBUG_SET_LEVEL_2)
        printf("  appending obj_id %s to file...\n", id);

    ret = self->data->add(self->data, id, &offset);

    /* indexing */
    if (self->num_objs % GLB_LEAF_SIZE == 0) {
        ret = self->index->addElem(self->index, id, offset);
        ret = self->index->update(self->index, self->index->root);
        self->num_objs = 0;
    }
    self->index->array[self->index->node_count - 1].id_last = id;

    self->num_objs++;
    
    return glb_OK;
}

static int 
glbSet_del(struct glbSet *self)
{
    if (!self) return glb_OK;
    
    if (self->data) self->data->del(self->data);
    if (self->index) self->index->del(self->index);

    free(self);

    return glb_OK;
}

static int
glbSet_init(struct glbSet *self, 
	    const char *path,
	    const char *name)
{
    int ret;
    self->path = path;
    self->name = name;

    ret = self->data->init(self->data, path, name);
    if (ret != glb_OK) return ret;

    return glb_OK;
}

int glbSet_new(struct glbSet **rec)
{
    int ret;
    struct glbSet *self;

    self = malloc(sizeof(struct glbSet));
    if (!self) return glb_NOMEM;

    memset(self, 0, sizeof(struct glbSet));

    /* creating inner structs */
    ret = glbSetFile_new(&self->data);
    if (ret) goto error;
    self->data->set = self;


    ret = glbIndexTree_new(&self->index);
    if (ret) goto error;

    /* init */
    self->init = glbSet_init;
    self->del = glbSet_del;
    self->add = glbSet_add;
    self->lookup = glbSet_lookup;

    *rec = self;
    return glb_OK;

error:

    glbSet_del(self);
    return ret;
}
