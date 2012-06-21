#ifndef GLB_STORAGE_H
#define GLB_STORAGE_H

#define GLB_NUM_AGENTS 1

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

    struct ooDict *obj_index;

    struct ooDict *search_cache;

    /**********  interface methods  **********/
    int (*del)(struct glbStorage *self);
    int (*str)(struct glbStorage *self);

    int (*start)(struct glbStorage *self);

    /*int (*add)(struct glbStorage *self, 
      struct glbData *data);

      int (*get)(struct glbStorage *self, 
      struct glbData *data); */

};

extern int glbStorage_new(struct glbStorage **self, 
			  const char *config);
#endif
