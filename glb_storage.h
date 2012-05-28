#ifndef GLB_STORAGE_H
#define GLB_STORAGE_H

#define GLB_NUM_AGENTS 4

struct agent_args {
    int agent_id;
    struct glbStorage *storage;
};


struct glbStorage
{
    char *name;
    char *path;
    char *service_address;

    char *cur_id; /* next obj id */
    char *key_id;

    char **id_pool;
    size_t id_count;

    void *context;

    size_t num_agents;

    /* storage partitions */
    struct glbPartition **partitions;

    /* concept indices for each agent/thread */
    struct glbMaze **mazes;

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
