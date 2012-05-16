#include <stdio.h>
#include <stdlib.h>

#include "glb_config.h"

typedef struct glbAddRequest
{
    struct glbSet **set_pool;
    
    char *source_string;
    size_t source_string_size;
    char *concept_graphs;
    
    char *concept_index;

    /***** temp area for test ******/
    



} glbRequest;

typedef struct glbSearchRequest
{
    struct glbSet **set_pool;

    char *source_string;
    size_t source_string_size;
    char *concept_graph;
    
    char *concept_index;

    size_t result_size;

} glbSearchRequest;

typedef struct glbColl
{
    char *name;
    char *env_path;

    char *cur_id; /* next obj id */
    char *key_id;
    char **id_pool;
    size_t id_count;

    /* concept index */
    struct glbMaze *maze;

    /* set index (name: set) */
    struct ooDict *set_index;
    
    /* (ID : URL) dictionary */
    struct ooDict *storage;
    
    /**********  interface methods  **********/
    int (*del)(struct glbColl *self);
    int (*str)(struct glbColl *self);
   
    int (*add)(struct glbColl *self,
	       const char *spec,
	       const char *obj);

    int (*find)(struct glbColl *self,
		const char *spec,
		const char *request);



} glbColl;

extern const char* glbColl_process(struct glbColl *self, const char *msg); 
extern int glbColl_new(struct glbColl **self);
