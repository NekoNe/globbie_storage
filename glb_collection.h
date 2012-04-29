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

typedef struct glbCollection
{
    char *name;
    char *env_path;

    char *cur_id; /* next document id */
    char *key_id;
    char **id_pool;
    size_t id_count;

    /* Dictionary with sets and their names(keys) (name : set) */
    struct ooDict *dict;
    
    /* (ID : URL) dictionary */
    struct ooDict *storage;
    
    /**********  interface methods  **********/
    
    int (*newDocument)(struct glbCollection *self, char *bytecode, size_t bytecode_size);  
    int (*findDocuments)(struct glbCollection *self, char *bytecode, size_t bytecode_size);

    int (*init)(struct glbCollection *self);
    int (*del)(struct glbCollection *self);
    int (*print)(struct glbCollection *self);

    /**********  internal methods  **********/

    int (*tconcept_to_sets)(struct glbCollection *self, char *index, struct glbSet **pool);   
    int (*tconcept_transform)(struct glbCollection *self, char *index, struct glbSet **pool);   
    
    int (*newSet)(struct glbCollection *self, char *name, struct glbSet **answer);
    int (*findSet)(struct glbCollection *self, char *name, struct glbSet **set); 
    
    int (*addDocToSet)(struct glbCollection *self, char *name);
    /* makes response */
    int (*encode)(struct glbCollection *self, char *result, size_t result_size, char *bytecode, size_t bytecode_size);
    
    /* decode request */
    int (*decode)(struct glbCollection *self, char *bytecode, size_t bytecode_size, char **name, char *url); 

} glbCollection;

extern char * glbCollection_process(struct glbCollection *self, char *msg); 
extern int glbCollection_new(struct glbCollection **self);
