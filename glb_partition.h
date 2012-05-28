#ifndef GLB_PARTITION_H
#define GLB_PARTITION_H


struct glbObjRecord 
{
    const char *meta;
    void *obj;
    const char *text;
    
};

struct glbPartition
{
    int id;
    const char *path;

    char *service_address;

    char *curr_obj_id;
  

    struct ooDict *obj_index;

    size_t num_objs;
    size_t max_num_objs;

    /**********  interface methods  **********/
    int (*del)(struct glbPartition *self);
    int (*str)(struct glbPartition *self);

    int (*update)(struct glbPartition *self);
   
    int (*add)(struct glbPartition *self,
	       char *spec,
	       size_t spec_size,
	       void *obj,
	       size_t obj_size,
	       char *text,
	       size_t text_size,
	       const char **obj_id);
};

extern int glbPartition_new(struct glbPartition **self);
#endif
