#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glb_config.h"
#include "glb_request_handler.h"
#include "glb_set_file.h"
#include "glb_set.h"
#include "glb_index_tree.h"

#define GLB_DEBUG_REQUEST_LEVEL_1 0
#define GLB_DEBUG_REQUEST_LEVEL_2 0

static int
glbIntersectionTable_solve(struct glbIntersectionTable *self)
{
    return glb_OK;
}

static int
glbIntersectionTable_refresh(struct glbIntersectionTable *self)
{
    size_t i, j;

    /*for (i = 0; i < self->answer_size * GLB_ID_MATRIX_DEPTH; i++) 
      self->ids[i] = (char)0; */
    
    self->answer_actual_size = 0;

    return glb_OK;
}

static int 
glbIntersectionTable_del(struct glbIntersectionTable *self)
{
    size_t i;

    if (!self) return glb_OK;
    if (self->ids) free(self->ids);


    free(self);

    return glb_OK;
}

static int 
glbIntersectionTable_init(struct glbIntersectionTable *self, 
			  size_t answer_size, 
			  size_t set_pool_size)
{
    self->answer_size = answer_size;
    self->answer_actual_size = 0;
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

    self->ids = malloc(answer_size * GLB_ID_SIZE + 1);
    if (!self->ids) { result = glb_NOMEM; goto error; }
   
    memset(self->ids, ' ', answer_size * GLB_ID_SIZE);
    self->ids[answer_size * GLB_ID_SIZE] = '\0';


    self->init = glbIntersectionTable_init;
    self->del = glbIntersectionTable_del;
    self->refresh = glbIntersectionTable_refresh;

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

    /*printf("  get next id (buf head: %d, actual size: %d)\n",
      self->zero_buffer_head, self->zero_buffer_actual_size); */

    if (self->zero_buffer_head >= self->zero_buffer_actual_size)
        return glb_EOB;

    offset = self->zero_buffer_head * GLB_ID_SIZE;

    memcpy(self->id,
	   self->zero_buffer + offset, 
	   GLB_ID_SIZE);

    self->zero_buffer_head++;

    return glb_OK;
}

static int
glbRequestHandler_init_buf(struct glbRequestHandler *self)
{
    struct glbSet *set;
    size_t res = 0;
    int err;
    
    set = self->set_pool[0];
    
    if (GLB_DEBUG_REQUEST_LEVEL_2)
	printf("   ..init read from %d...\n", self->offset);

    err = set->read_buf(set, 
			self->offset, 
			self->zero_buffer, 
			GLB_LEAF_SIZE * GLB_ID_SIZE, 
			&res);
    if (err) return err;

    if (!res) return glb_EOB;

    if (GLB_DEBUG_REQUEST_LEVEL_2)
	printf("INIT IDS: %s batch size: %d\n", 
	       self->zero_buffer, res);

    self->zero_buffer_actual_size = res / GLB_ID_SIZE;
    self->zero_buffer_head = 0;
    self->zero_buffer_tail = 0;
    self->offset += res;

    return glb_OK;
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

    size_t res;
    int comp_res;
    struct glbIntersectionTable *tmp;
    struct glbSet *set;

    /* copy zero_buffer into intersection_table */
    for (i = self->zero_buffer_tail, j = 0; i < self->zero_buffer_head; i++, j++) {
        
        /* copy ids to int_table */
        memcpy(self->int_table->ids + j * GLB_ID_SIZE, 
	       self->zero_buffer + i * GLB_ID_SIZE,
	       GLB_ID_MATRIX_DEPTH); 

    }

    self->int_table->answer_actual_size = j;

    /*printf("  actual size: %d...\n", j);*/
   
    /* intersect the int_table with buffered set[k] and writing result into res_table */
    for (k = 1; k < self->set_pool_size; k++) {

	set = self->set_pool[k];

	/*printf("set %d: offset: %d\n", k, self->prev_nodes[k]);
	  set->str(set);*/

        /* loading set[k] to swap buffer */
        set->read_buf(set, 
		      0, /*self->prev_nodes[k]->offset, */
		      self->swap, 
		      GLB_ID_SIZE * GLB_LEAF_SIZE, 
		      &res);

	/*printf("swap: %s\n", self->swap);*/

        self->swap_size = res;
        self->swap_size = self->swap_size / GLB_ID_SIZE;


        /* intersecting */
        res_table_cursor = 0;
        swap_cursor = 0;

        glbIntersectionTable_refresh(self->res_table);

        for (i = 0; i < self->int_table->answer_actual_size; i++) {
            for (j = swap_cursor; j < self->swap_size; j++) {
                comp_res = compare(self->int_table->ids + i * GLB_ID_MATRIX_DEPTH, 
				   self->swap + j * GLB_ID_SIZE);
                if (comp_res != glb_EQUALS) continue;
                memcpy(self->res_table->ids + res_table_cursor * GLB_ID_MATRIX_DEPTH, 
		       self->swap + j * GLB_ID_SIZE, 
		       GLB_ID_MATRIX_DEPTH);


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

    /*printf(" final results... actual: %d  total: %d\n",
	   self->result->answer_actual_size,
	   self->result->answer_size);*/

    while ((self->result->answer_actual_size < self->result->answer_size) &&\
	   (i < self->int_table->answer_actual_size)) {

        memcpy(self->result->ids + self->result->answer_actual_size * GLB_ID_SIZE, 
	       self->int_table->ids + i * GLB_ID_SIZE, 
	       GLB_ID_SIZE);

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
     *  II. Finding id in indexes and setting new positions in indexes 
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

    /*printf("next id: \"%s\"\n", self->id);*/

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
            if (glbRequestHandler_init_buf(self)) {
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

	if (GLB_DEBUG_REQUEST_LEVEL_2)
	    printf("   ++ lookup id: %s\n", self->id);

        res = self->lookup(self, self->set_pool[i]->index->root, self->id, &tmp);

	if (GLB_DEBUG_REQUEST_LEVEL_2)
	    printf("    == result: %d\n", res);

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

    self->result_size = result_size;
    self->offset = GLB_ID_SIZE * offset;

    self->zero_buffer_actual_size = 0;
    self->zero_buffer_head = 0;
    self->zero_buffer_tail = 0;

    self->set_pool = set_pool;
    self->set_pool_size = set_pool_size;
    
    self->swap_size = 0;

    for (i = 0; i < set_pool_size; i++) {
        self->cur_nodes[i] = set_pool[i]->index->root; 
        self->prev_nodes[i] = NULL;
    }

    self->int_table->init(self->int_table, result_size, set_pool_size);
    self->res_table->init(self->res_table, result_size, set_pool_size);
    self->result->init(self->result, result_size, set_pool_size);

    return glb_OK;
}

int glbRequestHandler_new(struct glbRequestHandler **rec)
{
    struct glbRequestHandler *self;
    int ret;

    self = NULL;

    self = malloc(sizeof(struct glbRequestHandler));
    if (!self) return glb_NOMEM;

    memset(self, 0, sizeof(struct glbRequestHandler));

    /* constructing inner structs */
    ret = glbIntersectionTable_new(&self->int_table, GLB_LEAF_SIZE, GLB_MAZE_NUM_AGENTS);
    if (ret) goto error;

    ret = glbIntersectionTable_new(&self->res_table, GLB_LEAF_SIZE,  GLB_MAZE_NUM_AGENTS);
    if (ret) goto error;

    ret = glbIntersectionTable_new(&self->result, GLB_RESULT_BATCH_SIZE, GLB_MAZE_NUM_AGENTS);
    if (ret) goto error;

    /* buffers */
    self->zero_buffer = malloc(GLB_LEAF_SIZE * GLB_ID_SIZE + 1);   
    if (!self->zero_buffer) { ret = glb_NOMEM; goto error; }
    memset(self->zero_buffer, ' ', GLB_LEAF_SIZE * GLB_ID_SIZE);
    self->zero_buffer[GLB_LEAF_SIZE * GLB_ID_SIZE] = '\0';

    self->swap = malloc(GLB_LEAF_SIZE * GLB_ID_SIZE + 1);
    if (!self->swap) { ret = glb_NOMEM; goto error; }
    memset(self->swap, ' ', GLB_LEAF_SIZE * GLB_ID_SIZE);
    self->swap[GLB_LEAF_SIZE * GLB_ID_SIZE] = '\0';

    self->cur_nodes = malloc(GLB_MAZE_NUM_AGENTS * sizeof(struct glbIndexTreeNode *)); 
    if (!self->cur_nodes) { ret = glb_NOMEM; goto error; }
 
    self->prev_nodes = malloc(GLB_MAZE_NUM_AGENTS * sizeof(struct glbIndexTreeNode *)); 
    if (!self->prev_nodes) { ret = glb_NOMEM; goto error; }


    /* current id */
    self->id = malloc((GLB_ID_MATRIX_DEPTH + 1) * sizeof(char));
    if (!self->id) { ret = glb_NOMEM; goto error; }
    self->id[GLB_ID_MATRIX_DEPTH] = '\0';



    self->init = glbRequestHandler_init;
    self->del = glbRequestHandler_del;
    self->next_id = glbRequestHandler_next_id;
    self->lookup = glbRequestHandler_lookup;
    self->leaf_intersection = glbRequestHandler_leaf_intersection;
    self->intersect = glbRequestHandler_intersect;
    
    /*ret = glbRequestHandler_init(self, result_size, offset, set_pool, GLB_MAZE_NUM_AGENTS);
      if (ret) goto error;*/
    
    *rec = self;


    return glb_OK;

error:
    glbRequestHandler_del(self);
    return ret;
}

