
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glb_config.h"
#include "glb_set.h"
#include "glb_set_file.h"
#include "glb_index_tree.h"

#define GLB_DEBUG_SET_LEVEL_1 0
#define GLB_DEBUG_SET_LEVEL_2 0
#define GLB_DEBUG_SET_LEVEL_3 0
#define GLB_DEBUG_SET_LEVEL_4 0
#define GLB_DEBUG_SET_LEVEL_5 0

static int 
glbSet_str(struct glbSet *self)
{
    struct glbSet *ref;

    printf("    == Set \"%s\" [num obj ids: %d]", 
	   self->name, self->num_obj_ids);

    if (self->refs) {

	ref = self->refs;

	printf("    REFS: ");
	while (ref) {
	    printf("[%s:%d] ", 
		   ref->name, ref->num_obj_ids);
	    ref = ref->next_ref;
	}
	puts("");
    }

    return glb_OK;
}

static int 
glbSet_lookup(struct glbSet *self, 
	      const char *id)
{
    size_t offset;
    int ret;

    if (GLB_DEBUG_SET_LEVEL_3) 
        printf("Looking up id \"%s\" in the index...\n", id);


    /*ret = self->index->lookup(self->index, id, &offset, self->index->root); 
    if (ret == glb_NO_RESULTS) return ret;

    if (GLB_DEBUG_SET_LEVEL_4) 
        printf("Reading offset \"%d\" from the data file...\n", offset);
   
    ret = self->data->lookup(self->data, id, offset);
    if (ret == glb_NO_RESULTS) return ret;

    if (GLB_DEBUG_SET_LEVEL_3)
        printf("Lookup success!\n");
    */

    return glb_OK;
}


static int
glbSet_read_buf(struct glbSet *self, 
		size_t offset, 
		char *buf, 
		size_t size, 
		size_t *result)
{
    const char *chunk;
    long remainder = 0;
    long chunk_size = 0;
    size_t res;

    if (GLB_DEBUG_SET_LEVEL_3)
	printf("  ..read buf of size %d [offset: %d]\n", 
	       size, offset);

    if (offset >= self->obj_ids_size) {
	*result = 0;
	return glb_FAIL;
    }

    chunk = self->obj_ids + offset;
    
    remainder = self->obj_ids_size - offset;

    if (remainder < size) 
	chunk_size = remainder;
    else
	chunk_size = size;

    if (GLB_DEBUG_SET_LEVEL_3)
	printf("   ++ ids chunk size: %d\n", chunk_size);

    memcpy(buf, chunk, chunk_size);
    *result = chunk_size; 

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

    /*ret = self->data->add(self->data, id, &offset);

    if (self->num_objs % GLB_LEAF_SIZE == 0) {
        ret = self->index->addElem(self->index, id, offset);
        ret = self->index->update(self->index, self->index->root);
        self->num_objs = 0;
    }

    self->index->array[self->index->num_nodes - 1].id_last = id;

    self->num_objs++;
    */

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
glbSet_build_index(struct glbSet *self)
{
    struct glbIndexTreeNode *node;
    const size_t chunk_size = GLB_ID_SIZE * GLB_LEAF_SIZE;
    char id[GLB_ID_MATRIX_DEPTH + 1];

    long offset = 0;
    long total_size = 0;
    const char *curr_id;
    int i, ret;

    id[GLB_ID_SIZE] = '\0';

    /*if (GLB_DEBUG_SET_LEVEL_3) */

    /*printf("build index for ids (num: %d)\n", 
      self->num_obj_ids);  */

    total_size = self->num_obj_ids * GLB_ID_SIZE;

    /*i = 0;
    for (offset = 0; offset < total_size; offset += chunk_size) {
	curr_id = self->obj_ids + offset;
	memcpy(id, curr_id, GLB_ID_SIZE);

	printf("%d) total: %d id: %s: offset: %d (num nodes: %d)\n", 
	       i, total_size, id, offset, self->index->num_nodes);

	i++;
	}*/

    for (offset = 0; offset < total_size; offset += chunk_size) {

	curr_id = self->obj_ids + offset;

	memcpy(id, curr_id, GLB_ID_SIZE);

	/*printf("%s\n", id);*/

	/*
	if (GLB_DEBUG_SET_LEVEL_4)
	printf("  .. index add id \"%s\" with offset %d\n", id, offset); */

	ret = self->index->addElem(self->index, (const char*)id, offset);
	if (ret != glb_OK) return ret;

	node = &self->index->array[self->index->num_nodes - 1];
	node->id_last = node->id;

    }

    /*printf("total: %d offset: %d\n", total_size, offset);*/

    return glb_OK;
}



static int
glbSet_init(struct glbSet *self)
{
    int ret;

    ret = self->index->init(self->index);
    if (ret != glb_OK) return ret;

    self->refs = NULL;
    self->next_ref = NULL;
    self->obj_ids = NULL;
    self->num_obj_ids = 0;
    self->obj_ids_size = 0;

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
    /*ret = glbSetFile_new(&self->data);
    if (ret) goto error;
    self->data->set = self;
    */

    ret = glbIndexTree_new(&self->index);
    if (ret) goto error;

    /* init */
    self->init = glbSet_init;
    self->del = glbSet_del;
    self->str = glbSet_str;
    self->add = glbSet_add;
    self->lookup = glbSet_lookup;
    self->read_buf = glbSet_read_buf;
    self->build_index = glbSet_build_index;

    *rec = self;
    return glb_OK;

error:

    glbSet_del(self);
    return ret;
}
