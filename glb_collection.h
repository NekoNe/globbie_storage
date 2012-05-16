#include <stdio.h>
#include <stdlib.h>

#include "glb_config.h"

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

    int (*get)(struct glbColl *self,
	       const char *spec);

    int (*find)(struct glbColl *self,
		const char *spec,
		const char *request);

    int (*remove)(struct glbColl *self,
		  const char *spec);



} glbColl;

extern const char* glbColl_process(struct glbColl *self, const char *msg); 
extern int glbColl_new(struct glbColl **self);
