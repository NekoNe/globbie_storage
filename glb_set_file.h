#ifndef GLB_SET_FILE_H
#define GLB_SET_FILE_H

#include <stdio.h>
#include <stdlib.h>

#include "glb_config.h"


struct glbIndexTree;

typedef struct glbSetFile
{
    const char *path;
    const char *name;

    char *filename;

    struct glbSet *set;

    /* memory to do operations */
    char *buffer;
    size_t buffer_size;

    /**********  public methods  **********/

    int (*init)(struct glbSetFile *self, 
		const char *path, 
		const char *name);

    int (*del)(struct glbSetFile *self); /* destructor */

    /* contains part of set file to buffer */ 
    int (*read_buf)(struct glbSetFile *self, 
		    size_t offset, 
		    char *buf, 
		    size_t buf_size, 
		    size_t *buf_read_size);
    
    /* looking up obj_id */
    int (*lookup)(struct glbSetFile *self, const char *id, size_t offset);

    /* appending id  */
    int (*add)(struct glbSetFile *self, const char *id, size_t *offset);

    /* read index from file  */
    int (*read)(struct glbSetFile *self, 
		struct glbIndexTree *index);

} glbSetFile;

/* constructor */
extern int glbSetFile_new(struct glbSetFile **self);


#endif
