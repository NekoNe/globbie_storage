
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "glb_config.h"


/**
  *     PACKED = {ID:GLB_ID_MATRIX_DEPTH chars, DATA_SIZE:size_t, DATA: size_t * chars } 
  *     
  */

static int 
glbSetElem_unpack(struct glbSetElem *self, char *buff)
{
    size_t offset;

    offset = (size_t)0;
    memcopy(self->id, buff, (size_t)(GLB_ID_MATRIX_SIZE * sizeof(char)));
    
    offset += (size_t)(GLB_ID_MATRIX_SIZE * sizeof(char));
    self->locset->bytecode_size = *(size_t *)(buff + offset); /* is it ok */

    offset += sizeof(size_t);
    memcopy(self->locset->bytecode, buff + offset, (size_t)(self->locset->bytecode_size * sizeof(char))); 

    return glb_OK;
}

static int 
glbSetElem_pack(struct glbSetElem *self, char *buff, size_t *buff_size, size_t *actual_size)
{
    size_t bsize;
    size_t offset;
/* TODO */
    bsize = GLB_ID_MATRIX_DEPTH * sizeof(char) + sizeof(size_t) + self->locset->locset_size;
    if (bsize > *buff_size) return glb_NOMEM;

    memcpy(buff, self->id, GLB_ID_MATRIX_DEPTH * sizeof(char));
    offset = GLB_ID_MATRIX_DEPTH * sizeof(char);
    
    memcpy(buff, self->id, GLB_ID_MATRIX_DEPTH * sizeof(char));
    offset = GLB_ID_MATRIX_DEPTH * sizeof(char);
    
    return glb_OK;
}

static int
glbSetElem_del(struct glbSetElem *self)
{
    /* TODO */
    return glb_OK;
}



static int
glbSetElem_init(struct glbSetElem *self)
{
    self->id = {'0', '0', '0'}; /* fix it! I don't know id size */
    self->lockset = NULL;

    self->init = glbSetElem_init;
    self->del = glbSetElem_del;
    self->pack = glbSetElem_pack;

    return glb_OK;
}

int glbSetElem_new(struct glbSetElem **rec)
{
    struct glbSetElem *self;

    self = malloc(sizeof(struct glbSetElem));
    if (!self) return glb_NOMEM;

    glbSetElem_init(self);

    *rec = self; 
    return glb_OK;
}


