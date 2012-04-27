#include <stdio.h>
#include <stdlib.h>

#include "glb_config.h"

typedef struct glbSetFile
{
    char path_set[GLB_NAME_LENGTH];
    char path_locset[GLB_NAME_LENGTH];
   
    /* memory to do operations */
    char *buffer;
    size_t buffer_size;

    /**********  public methods  **********/

    int (*init)(struct glbSetFile *self, const char *name, const char *env_path);
    int (*del)(struct glbSetFile *self); /* destructor */

    /* contains part of set file to buffer */ 
    int (*buffering)(struct glbSetFile *self, size_t offset, char *buffer, size_t size, size_t *result);
    
    /* looking up id and getting it's locset offset */
    int (*lookup)(struct glbSetFile *self, const char *id, size_t *offset);
    
    /* looking up id in buffer and getting locset offset */
    int (*lookupInBuffer)(struct glbSetFile *self, const char *id, size_t *offset, size_t buff_size); 
    
    /* writing locset into locset file */
    int (*addByteCode)(struct glbSetFile *self, const char *bytecode, size_t bytecode_size, size_t *offset);
    /*writing id and locset offset into set file */
    int (*addId)(struct glbSetFile *self, const char *id, size_t locset_p, size_t *offset);

    /* init files */
    int (*create_files)(struct glbSetFile *self);

} glbSetFile;

/* constructor */
extern int glbSetFile_new(struct glbSetFile **self, const char *name, const char *env_path);

