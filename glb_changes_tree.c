
#include <stdio.h>
#include <stdlib.h>

#include "glb_config.h"
#include "glb_changes_tree.h"
#include "glb_set_elem.h"

static int
glbChangesTree_lookupElem(struct glbChangesTree *self, const char *id, struct glbSetElem *result)
{
    size_t offset;
    size_t pointer_offset;
    size_t locset_size;
    int rec;

    rec = self->lookup(self, id, &offset, &pointer_offset);
    
    if (rec) return glb_NO_RESULTS;
 
    memcpy(result->id, self->buffer + offset, GLB_ID_MATRIX_DEPTH * sizeof(char));
    offset += (size_t)GLB_ID_MATRIX_DEPTH;
    memcpy(&locset_size, self->buffer + offset, sizeof(size_t));
    offset += sizeof(size_t);
    memcpy(result->locset->bytecode, self->buffer + offset, locset_size);

    return glb_OK;
}

static int 
glbChangesTree_changeElem(struct glbChangesTree *self, struct glbSetElem *elem)
{
    size_t offset;
    size_t pointer_offset;
    int rec;

    if (!self->lookup(self, elem->id, &offset, &pointer_offset)) {
        rec = self->write(self, elem, glb_PRESENCE, &pointer_offset);
        if (rec) return glb_NOMEM;
        self->changeState(self, offset, glb_IRRELEVANT);
        return glb_OK;
    }
    return glb_NO_RESULTS;
}

static int
glbChangesTree_addElem(struct glbChangesTree *self, struct glbSetElem *elem)
{
    size_t offset;
    size_t pointer_offset;

    if (!self->lookup(self, elem->id, &offset, &pointer_offset)) {
        self->changeState(self, offset, glb_PRESENCE);
        return glb_OK;
    }
    return glb_NO_RESULTS;
}

static int
glbChangesTree_delElem(struct glbChangesTree *self, struct glbSetElem *elem)
{
    size_t offset;
    size_t pointer_offset;

    if (!self->lookup(self, elem->id, &offset, &pointer_offset)) {
        self->changeState(self, offset, glb_DELETED);
        return glb_OK;
    }
    return self->write(self, elem, glb_DELETED, &pointer_offset);
}

static int
glbChangesTree_write(struct glbChangesTree *self, struct glbSetElem *elem, int status, size_t pointer_offset)
{
    size_t i, j;
    size_t elem_size;
    size_t zero;

    i = self->buffer_end;
    zero = (size_t)0;
    /* let's write new data at the end of file */
    /* but has buffer enough space? */ 
    
    /* block size calculation */
    elem_size = (size_t)GLB_ID_MATRIX_DEPTH + (size_t)sizeof(int) + \
                (size_t)sizeof(size_t) + elem->locset->bytecode_size + 2*sizeof(size_t); 
    
    if (self->buffer_end + elem_size > buffer_size) return glb_NOMEM;

    /* ok. let's write */
    /*ID*/
    memcpy(self->buffer + i, elem->id, GLB_ID_MATRIX_DEPTH);
    i += GLB_ID_MATRIX_DEPTH;

    /*status*/
    memcpy(self->buffer + i, &status, sizeof(int));
    i += (size_t)sizeof(int);

    /*locset size*/
    memcpy(self->buffer + i, &(elem->locset->bytecode_size, sizeof(size_t));
    i += (size_t)sizeof(size_t);

    /*locset*/
    memcpy(self->buffer + i, elem->locset->bytecode, elem->locset->bytecode_size);
    i += elem->locset->bytecode_size;

    /* left and right markers */
    memcpy(self->buffer + i), &zero, sizeof(size_t));
    i += sizeof(size_t);
    memcpy(self->buffer + i, &zero, sizeof(size_t));

    /* change pointer to this new record */
    memcpy(self->buffer + pointer_offset, &(self->buffer_end, sizeof(size_t)));
    buffer_end += elem_size;
    
    return glb_OK;
}

static int
glbChangesTree_changeState(struct glbChangesTree *self, size_t offset, int status)
{
    memcpy(self->buffer + offset + GLB_ID_MATRIX_DEPTH, &status, sizeof(int));
    return glb_OK;
}

/*  for better performance 
 *  I expect no mtfk damaged the buffer
 *  NOTE: no variable hasn't changed during iteration
 *  means somebody crashed the buffer
 */
static int
glbChangesTree_lookup(struct glbChangesTree *self, const char *id, size_t *offset, size_t *pointer_offset)
{
    size_t i;
    size_t *size_t_p;
    size_t right, left;
    size_t locset_size;
             
    int comparison;
    int state;

    i = (size_t)0;

    while (1) {
        comparison = compare(id, self->buffer + i);

        if (!comparison) {
            memcpy(&state, self->buffer + i + (size_t)GLB_ID_MATRIX_DEPTH, sizeof(int));
            if (state != glb_IRRELEVANT) {
                *offset = i;
                return glb_OK;
            }
        }

        i += (size_t)GLB_ID_MATRIX_DEPTH; /* jump over ID field */

        i += (size_t)sizeof(int); /* jump over stutus filed */
        
        size_t_p = (size_t *)(self->buffer + i); /* jump over locset size */
        locset_size = *size_t_p;
        
        i += locset_size; 

        size_t_p = (size_t *)(self->buffer + i);
        left = *size_t_p;   /* reading left offset */

        if (comparison == glb_LESS) {
            if (!left) {
                *pointer_offset = i;
                return glb_NO_RESULTS;
            }
            i = left;
        }
    
        i += (size_t)sizeof(size_t); /* jump over left field */
        size_t_p = (size_t *)(self->buffer + i); 
        right = *size_t_p;   /* reading right offset */
    
        if (!right) {
            *pointer_offset = i;
            return glb_NO_RESULTS;
        }
        i = right;
    }
}

static int
glbChangesTree_del(struct glbChangestree *self)
{
    /*TODO*/
    return glb_OK;
}

static int
glbChangesTree_init(struct glbChangesTree *self)
{
    self->init = glbCangesTree_init;
    self->del = glbChangesTree_del;
    self->lookup = glbChangesTree_lookup;
    self->changeState = glbChngesTree_changeState;
    self->write = glbChangesTree_write;
    self->update = glbChangesTree_update;
    self->addElem = glbChangesTree_addElem;
    self->delElem = glbChangesTree_delElem;
    self->changeElem = glbChangesTree_changeElem;
    self->lookupElem = glbChangestree_lookupElem;

    return glb_OK;
}

int glbChangesTree_new(struct glbChangesTree **rec)
{
    struct glbChangesTree *self;

    self = malloc(sizeof(glbChangesTree));
    if (!self) return glb_NOMEM;

    glbChangesTree_init(self);

    *rec = self;

    return glb_OK;
}

