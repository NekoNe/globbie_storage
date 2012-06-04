#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glb_config.h"
#include "glb_request_handler.h"
#include "glb_set_file.h"
#include "glb_set.h"
#include "glb_index_tree.h"


static int
glbIntersectionTable_solve(struct glbIntersectionTable *self)
{
    return glb_OK;
}

static int
glbIntersectionTable_refresh(struct glbIntersectionTable *self)
{
    size_t i, j;

    for (i = 0; i < self->answer_size * GLB_ID_MATRIX_DEPTH; i++) 
        self->ids[i] = (char)0;
    
    for (i = 0; i < self->set_pool_size; i++)
        for (j = 0; j < self->answer_size; j++)
            self->locsets[i][j] = 0;
  
    self->answer_actual_size = 0;

    return glb_OK;
}

static int 
glbIntersectionTable_del(struct glbIntersectionTable *self)
{
    size_t i;

    if (!self) return glb_OK;
    if (self->ids) free(self->ids);
    if (self->locsets) {
        for (i = 0; i < self->set_pool_size; i++) 
            if (self->locsets[i]) free(self->locsets[i]);
        free(self->locsets);
    }
    free(self);

    return glb_OK;
}

static int 
glbIntersectionTable_init(struct glbIntersectionTable *self, size_t answer_size, size_t set_pool_size)
{
    self->init = glbIntersectionTable_init;
    self->del = glbIntersectionTable_del;
    self->refresh = glbIntersectionTable_refresh;

    self->answer_size = answer_size;
    self->set_pool_size = set_pool_size;

    return glb_OK;
}

int glbIntersectionTable_new(struct glbIntersectionTable **rec, 
			     size_t answer_size, 
			     size_t set_pool_size)
{
    struct glbIntersectionTable *self;
    int result;
    size_t i;
    
    self = malloc(sizeof(struct glbIntersectionTable));
    if (!self) return glb_NOMEM;

    memset(self, 0, sizeof(struct glbIntersectionTable));

    self->ids = malloc(answer_size * GLB_ID_MATRIX_DEPTH * sizeof(char));
    if (!self->ids) { result = glb_NOMEM; goto error; }
   
    self->locsets = malloc(set_pool_size * sizeof(size_t *));
    if (!self->locsets) { result = glb_NOMEM; goto error; }
   
    for (i = 0; i < set_pool_size; i++) {
        self->locsets[i] = malloc(answer_size * sizeof(size_t));
        if (!self->locsets[i]) { result = glb_NOMEM; goto error; }
    }

    result = glbIntersectionTable_init(self, answer_size, set_pool_size);
    if (result) goto error;

    *rec = self;
    return glb_OK;

error:
    glbIntersectionTable_del(self);
    return result;
}


static int
glbRequestHandler_next_id(struct glbRequestHandler *self)
{
    size_t offset;
    
    if (self->zero_buffer_head >= self->zero_buffer_actual_size) 
        return glb_EOB;

    offset = self->zero_buffer_head * GLB_ID_BLOCK_SIZE;

    memcpy(self->id,
	   self->zero_buffer + offset, 
	   GLB_ID_MATRIX_DEPTH);

    self->zero_buffer_head++;

    return glb_OK;
}

static int
glbRequestHandler_read_buf(struct glbRequestHandler *self)
{
    size_t res;
    int err;
    
    err = self->set_pool[0]->data->read_buf(self->set_pool[0]->data, 
					    self->offset, 
					    self->zero_buffer, 
					    GLB_LEAF_SIZE * GLB_ID_BLOCK_SIZE, 
					    &res);
    if (res) {
        self->zero_buffer_actual_size = res / GLB_ID_BLOCK_SIZE;
        self->zero_buffer_head = 0;
        self->zero_buffer_tail = 0;
        self->offset += res;

        return glb_OK;
    } 
    /* TODO: error handling */
     
    return glb_EOB;
}

static int
glbRequestHandler_lookup(struct glbRequestHandler *self, 
			 struct glbIndexTreeNode *marker, 
			 const char *id, 
			 struct glbIndexTreeNode **node)
{
    int left, right, res;

    /* TODO: jumping over tree */ 
    if (!marker)  
        return glb_NO_RESULTS;

    left = compare(marker->id, id);

    right = compare(marker->id_last, id);

    if (((left == glb_LESS) || (left == glb_EQUALS)) &&\
        ((right == glb_MORE) || (left == glb_EQUALS))) {
        *node = marker;
        return glb_OK;
    }

    if (left == glb_MORE) {
        return self->lookup(self, marker->left, id, node);
    }

    return self->lookup(self, marker->right, id, node);
}

static int
glbRequestHandler_leaf_intersection(struct glbRequestHandler *self) 
{
    size_t i, j, k, l, m;
    size_t res_table_cursor, swap_cursor;
    size_t locset;
    int res, comp_res;
    struct glbIntersectionTable *tmp;


    /* copy zero_buffer into intersection_table */
    for (i = self->zero_buffer_tail, j = 0; i < self->zero_buffer_head; i++, j++) {
        
        /* copy ids to int_table */
        memcpy(self->int_table->ids + j * GLB_ID_MATRIX_DEPTH, 
	       self->zero_buffer + i * GLB_ID_BLOCK_SIZE, 
	       GLB_ID_MATRIX_DEPTH); 

        /* copy locset offsets to int_table */
        memcpy(&locset, 
	       self->zero_buffer + i * GLB_ID_BLOCK_SIZE + GLB_ID_MATRIX_DEPTH, 
	       GLB_SIZE_OF_OFFSET);
        self->int_table->locsets[0][j] = locset;
    }
    self->int_table->answer_actual_size = j;
    
    /* intersection int_table with buffered set[k] and writing result into res_table */

    for (k = 1; k < self->set_pool_size; k++) {

        /* loading set[k] to swap buffer */
        self->set_pool[k]->data->read_buf(self->set_pool[k]->data, 
					  self->prev_nodes[k]->offset, 
					  self->swap, 
					  GLB_ID_BLOCK_SIZE * GLB_LEAF_SIZE, 
					  &res);
        self->swap_size = res;
        self->swap_size = self->swap_size / GLB_ID_BLOCK_SIZE;

        /* intersecting */
        res_table_cursor = 0;
        swap_cursor = 0;
        glbIntersectionTable_refresh(self->res_table);
        for (i = 0; i < self->int_table->answer_actual_size; i++) {
            for (j = swap_cursor; j < self->swap_size; j++) {
                comp_res = compare(self->int_table->ids + i * GLB_ID_MATRIX_DEPTH, 
				   self->swap + j * GLB_ID_BLOCK_SIZE);
                if (comp_res != glb_EQUALS) continue;
                memcpy(self->res_table->ids + res_table_cursor * GLB_ID_MATRIX_DEPTH, 
		       self->swap + j * GLB_ID_BLOCK_SIZE, 
		       GLB_ID_MATRIX_DEPTH);
                memcpy(&locset, 
		       self->swap + j * GLB_ID_BLOCK_SIZE + GLB_ID_MATRIX_DEPTH, 
		       GLB_SIZE_OF_OFFSET);
                self->res_table->locsets[k][res_table_cursor] = locset;
                res_table_cursor++;
                swap_cursor = j;
                self->res_table->answer_actual_size++;
                break;
            }
        }

        /* swaping int_table with res_table */
        tmp = self->int_table;
        self->int_table = self->res_table;
        self->res_table = tmp;
    }   

    /* writing this part of result to final */
    i = 0;
    while ((self->result->answer_actual_size < self->result->answer_size) &&\
	   (i < self->int_table->answer_actual_size)) {

        memcpy(self->result->ids + self->result->answer_actual_size * GLB_ID_MATRIX_DEPTH, 
	       self->int_table->ids + i * GLB_ID_MATRIX_DEPTH, 
	       GLB_ID_MATRIX_DEPTH);

        for (j = 0; j < self->set_pool_size; j++) {
            self->result->locsets[j][self->result->answer_actual_size] = self->int_table->locsets[j][i];
        }

        self->result->answer_actual_size++; 
        i++;
    }
    
    if (self->result->answer_actual_size == self->result->answer_size)
        return glb_STOP;

    return glb_OK;
}

static int
glbRequestHandler_intersect(struct glbRequestHandler *self)
{
    int res, changed, not_null;
    size_t i;

    struct glbIndexTreeNode *tmp;

    /*   I. Get new id from zero buffer (if it is possible)
     *  II. Finding id in indexes and setting new possitions in indexes 
     * III. If one of indexes has changed =>
     *          a. intersecting (по) old positions 
     *          b. adding this result to final result
     *          c. if final result is ready => stop 
     *          d. new positions = old position (indexes)
     *        
     *  IV. goto I
     */

    self->zero_buffer_actual_size = 0;
    self->zero_buffer_head = GLB_LEAF_SIZE;
    self->zero_buffer_tail = GLB_LEAF_SIZE;

    /*self->read_buf(self);*/

begin:
    changed = false;
    res = 0;

    /* I. new id */
    res = self->next_id(self);


    if (res == glb_EOB) {
         
        if (self->zero_buffer_head > self->zero_buffer_tail) {
            res = self->leaf_intersection(self);
            self->zero_buffer_head = GLB_LEAF_SIZE; /* TODO: not leaf size, but actual_size */
            self->zero_buffer_tail = GLB_LEAF_SIZE;
            if (res == glb_STOP)
                return glb_OK;
            goto begin;
        }

        if (self->zero_buffer_head == self->zero_buffer_tail) {
            if (self->read_buf(self)) {
                return glb_EOB;
            }
            self->next_id(self);
        }

        if (self->zero_buffer_head < self->zero_buffer_tail) {
            return glb_FAIL;
        }
    }

    /*printf("   ++ next id: %s\n", self->id);*/


    /* II. id search */
    for (i = 1; i < self->set_pool_size; i++) {

        res = self->lookup(self, self->set_pool[i]->index->root, self->id, &tmp);

        if (res == glb_NO_RESULTS) goto begin;
        self->cur_nodes[i] = tmp;

    }

    /* III. intersecting */
    /* finding out changes */
    changed = false;
    not_null = true;

    for (i = 1; i < self->set_pool_size; i++) {
        if (!self->prev_nodes[i]) not_null = false;
        if (self->prev_nodes[i] != self->cur_nodes[i]) changed = true;
    }

    /* if leaf (index tree node) changed => intersect */
    if ((changed) && (not_null)) {
        self->zero_buffer_head--;
        res = self->leaf_intersection(self);
        if (res == glb_STOP)
            return glb_OK;
        self->zero_buffer_tail = self->zero_buffer_head;
    }

    /* updating leaves */
    for (i = 1; i < self->set_pool_size; i++) {
        self->prev_nodes[i] = self->cur_nodes[i];
    }

    goto begin;

    /* we will never get here. buffer will end definitely */
    return glb_OK;
}

static int
glbRequestHandler_del(struct glbRequestHandler *self)
{
    if (!self) return glb_OK;
   
   
    if (self->res_table) self->res_table->del(self->res_table);
    if (self->int_table) self->int_table->del(self->int_table);
    if (self->result) self->result->del(self->result);
    
    if (self->zero_buffer) free(self->zero_buffer);
    if (self->swap) free(self->swap);

    if (self->cur_nodes) free(self->cur_nodes);
    if (self->prev_nodes) free(self->prev_nodes);

    free(self); 
    
    return glb_OK;
}

static int
glbRequestHandler_init(struct glbRequestHandler *self, 
		       size_t result_size, 
		       size_t offset, 
		       struct glbSet **set_pool,
		       size_t set_pool_size)
{
    size_t i;

    self->next_id = glbRequestHandler_next_id;
    self->read_buf = glbRequestHandler_read_buf;
    self->lookup = glbRequestHandler_lookup;
    self->leaf_intersection = glbRequestHandler_leaf_intersection;
    self->intersect = glbRequestHandler_intersect;
    
    self->init = glbRequestHandler_init;
    self->del = glbRequestHandler_del;

    self->result_size = result_size;
    self->offset = GLB_ID_BLOCK_SIZE * offset;

    self->zero_buffer_actual_size = 0;
    self->zero_buffer_head = 0;
    self->zero_buffer_tail = 0;

    self->set_pool_size = set_pool_size;
    self->set_pool = set_pool;
    
    self->swap_size = 0;

    for (i = 0; i < set_pool_size; i++) {
        self->cur_nodes[i] = set_pool[i]->index->root; 
        self->prev_nodes[i] = NULL;
    }

    return glb_OK;
}

int glbRequestHandler_new(struct glbRequestHandler **rec, 
			  size_t result_size, 
			  size_t offset, 
			  struct glbSet **set_pool,
			  size_t set_pool_size)
{
    struct glbRequestHandler *self;
    struct glbIntersectionTable *int_table,
                                *res_table,
                                *result;
    int res;

    self = NULL;

    self = malloc(sizeof(struct glbRequestHandler));
    if (!self) return glb_NOMEM;

    memset(self, 0, sizeof(struct glbRequestHandler));

    /* constructing inner structs */
    res = glbIntersectionTable_new(&int_table, GLB_LEAF_SIZE, set_pool_size);
    if (res) goto error;
    self->int_table = int_table;

    res = glbIntersectionTable_new(&res_table, GLB_LEAF_SIZE, set_pool_size);
    if (res) goto error;
    self->res_table = res_table;

    res = glbIntersectionTable_new(&self->result, result_size, set_pool_size);
    if (res) goto error;

    /* buffers */
    self->zero_buffer = malloc(GLB_LEAF_SIZE * GLB_ID_BLOCK_SIZE * sizeof(char));   
    if (!self->zero_buffer) { res = glb_NOMEM; goto error; }

    self->cur_nodes = malloc(set_pool_size * sizeof(struct glbIndexTreeNode *)); 
    if (!self->cur_nodes) { res = glb_NOMEM; goto error; }
    
    self->prev_nodes = malloc(set_pool_size * sizeof(struct glbIndexTreeNode *)); 
    if (!self->prev_nodes) { res = glb_NOMEM; goto error; }

    self->id = malloc((GLB_ID_MATRIX_DEPTH + 1) * sizeof(char));
    if (!self->id) { res = glb_NOMEM; goto error; }
    self->id[GLB_ID_MATRIX_DEPTH] = '\0';

    self->swap = malloc(GLB_ID_BLOCK_SIZE * GLB_LEAF_SIZE * sizeof(char));
    if (!self->swap) { res = glb_NOMEM; goto error; }

    res = glbRequestHandler_init(self, result_size, offset, set_pool, set_pool_size);
    if (res) goto error;
    
    *rec = self;

    return glb_OK;

error:
    glbRequestHandler_del(self);
    return res;
}

