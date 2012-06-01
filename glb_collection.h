#ifndef GLB_COLL_H
#define GLB_COLL_H

struct glbCollRef
{
    char *name;
    char *service_address;

    struct glbCollRef *next;
};


struct glbColl
{
    char *name;
    char *env_path;

    void *context;

    struct glbCollRef *children;

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

    int (*start)(struct glbColl *self);

    int (*find_route)(struct glbColl *self,
		      const char *spec,
		      const char **dest_coll_addr);


};

extern int glbColl_new(struct glbColl **self,
		       const char *config);
#endif
