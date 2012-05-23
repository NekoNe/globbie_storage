#ifndef GLB_STORAGE_H
#define GLB_STORAGE_H

struct glbStorage
{
    char *name;
    char *path;
    char *service_address;

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
    int (*del)(struct glbStorage *self);
    int (*str)(struct glbStorage *self);

    int (*update)(struct glbStorage *self);
   
    int (*add)(struct glbStorage *self,
	       const char *spec,
	       const char *obj);

    int (*get)(struct glbStorage *self,
	       const char *spec);

    int (*find)(struct glbStorage *self,
		const char *spec,
		const char *request);

    int (*remove)(struct glbStorage *self,
		  const char *spec);
    
    int (*start)(struct glbStorage *self);

};

extern int glbStorage_new(struct glbStorage **self, 
			  const char *config);
#endif
