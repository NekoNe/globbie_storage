
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glb_config.h"
#include "glb_set.h"
#include "glb_set_file.h"
#include "glb_index_tree.h"

static int 
glbSet_lookup(struct glbSet *self, const char *id, size_t *locset_pointer)
{
    size_t offset;
    int res;

    if (GLB_DEBUG_LEVEL_3) 
        printf("Looking up id \"%c%c%c\" in the index...\n", id[0], id[1], id[2]);

    res = self->index->lookup(self->index, id, &offset, self->index->root); 
    if (res == glb_NO_RESULTS) return glb_NO_RESULTS;

    if (GLB_DEBUG_LEVEL_4) 
        printf("Reading offset \"%d\" from the data file...\n", offset);
   
    res = self->data->lookup(self->data, id, &offset);
    if (res == glb_NO_RESULTS) return glb_NO_RESULTS;
    *locset_pointer = offset;

    if (GLB_DEBUG_LEVEL_3)
        printf("Lookup success! Offset: %d\n", *locset_pointer);

    return glb_OK;
}

static int
glbSet_add(struct glbSet *self, const char *id, const char *bytecode, size_t bytecode_size)
{
    int state;
    size_t offset;
    struct glbIndexTreeNode *last;
    
    /* saving to file */
    state = self->data->addByteCode(self->data, bytecode, bytecode_size, &offset);
    state = self->data->addId(self->data, id, offset, &offset);

    /* indexing */
    if (self->index_counter % GLB_LEAF_SIZE == 0) {
        state = self->index->addElem(self->index, id, offset);
        state = self->index->update(self->index, self->index->root);
        self->index_counter = 0;
    }
    self->index->array[self->index->node_count - 1].id_last = id;

    self->index_counter++;
    
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
glbSet_init(struct glbSet *self, const char *name, const char *env_path)
{
    self->init = glbSet_init;
    self->del = glbSet_del;
    self->add = glbSet_add;
    self->lookup = glbSet_lookup;

    self->index_counter = 0;

    /* setting name and path */
    if (strlen(name)  + 1 > GLB_NAME_LENGTH)
        return glb_FAIL;
    strcpy(self->name, name);

    if (strlen(env_path) + 1 > GLB_NAME_LENGTH)
        return glb_FAIL;
    strcpy(self->env_path, env_path);

    return glb_OK;
}

int glbSet_new(struct glbSet **rec, const char *name, const char *env_path)
{
    int result;
    struct glbSet *self;
    struct glbSetFile *data;
    struct glbIndexTree *index;

    self = malloc(sizeof(struct glbSet));
    if (!self) return glb_NOMEM;

    self->data = NULL;
    self->index = NULL;
        
    self->index_counter = 0;

    self->init = NULL;
    self->del = NULL;
    self->add = NULL;
    self->lookup = NULL;

    memset(self->name, 0, GLB_NAME_LENGTH);
    memset(self->env_path, 0, GLB_NAME_LENGTH);

    /* creating inner structs */
    result = glbSetFile_new(&data, name, env_path);
    if (result) goto error;
    self->data = data;

    result = glbIndexTree_new(&index);
    if (result) goto error;
    self->index = index;

    /* init */
    result = glbSet_init(self, name, env_path);
    if (result) goto error;

    *rec = self;
    return glb_OK;

error:
    glbSet_del(self);
    return result;
}
