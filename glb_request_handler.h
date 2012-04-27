#include <stdio.h>
#include <stdlib.h>

#include "glb_utils.h"
#include "glb_config.h"


/*  
 *  this table contains info about 
 *  position of concepts (sets) in docs (ids)
 *
 */

typedef struct glbIntersectionTable
{
    size_t set_pool_size;
    size_t answer_size;
    size_t answer_actual_size;

    char *ids;
    size_t **locsets;

    /********** public methods **********/

    int (*solve)(struct glbIntersectionTable *self); /* self intersection using locsets */
    int (*refresh)(struct glbIntersectionTable *self);

    int (*init)(struct glbIntersectionTable *self, size_t answer_size, size_t set_pool_size);
    /* destructor */
    int (*del)(struct glbIntersectionTable *self);

} glbIntersectionTable; 

extern int glbIntersectionTable_new(struct glbIntersectionTable **rec, size_t answer_size, size_t set_pool_size);


typedef struct glbRequestHandler
{
    char *id;

    struct glbIntersectionTable *int_table; /* buffer is copied there to intersect*/
    struct glbIntersectionTable *res_table; /* intermediate result */

    struct glbIntersectionTable *result;
    size_t result_size;

    /* set[0] is buffered here */
    char *zero_buffer;
    size_t zero_buffer_actual_size;
    size_t zero_buffer_head; /* left end of beffer */
    size_t zero_buffer_tail; /* right end of buffer */

    /* char *locset_buff; */

    char *swap; /* used in leaf_intersection to buffer set[i] */
    size_t swap_size;

    size_t offset; /* there search begins */

    struct glbSet **set_pool;
    size_t set_pool_size;

    /* contains positions of ids in indexes on cur step and prev step */
    struct glbIndexTreeNode **cur_nodes; 
    struct glbIndexTreeNode **prev_nodes;

    /*********** public methods **********/

    /* intersectin sets */ 
    int (*intersect)(struct glbRequestHandler *self);
    
    int (*init)(struct glbRequestHandler *self, size_t result_size,
                                                size_t offset,
                                                struct glbSet **set_pool,
                                                size_t set_pool_size);
    /* destructor */
    int (*del)(struct glbRequestHandler *self);

    /*********** internal mathods **********/

    /* takes next id from zero_buffer. writes it in *id */ 
    int (*next_id)(struct glbRequestHandler *self);
    /* loading new part of set to zero_buffer */
    int (*buffering)(struct glbRequestHandler *self);
    /* looking up IndexNode there id presents */
    int (*lookup)(struct glbRequestHandler *self, struct glbIndexTreeNode *marker, char *id, struct glbIndexTreeNode **node);
    /* intersects buffer of ids */
    int (*leaf_intersection)(struct glbRequestHandler *self);


} glbRequestHandler;

/* constructor */
extern int glbRequestHandler_new(struct glbRequestHandler **rec, size_t result_size,
                                                                 size_t offset,
                                                                 struct glbSet **set_pool,
                                                                 size_t set_pool_size);



