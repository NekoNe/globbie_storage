
#include <stdio.h>
#include <stdlib.h>

typedef struct glbSetFile
{
    const char *path_set;
    const char *path_locset;
    size_t file_size;
    
    char *buffer;
    size_t buffer_size;

    /**********  public methods  **********/
    int (*init)(struct glbSetFile *self);
    int (*del)(struct glbSetFile *self);

    int (*lookup)(struct glbSetFile *self, const char *id, size_t *offset);
    int (*lookupInBuffer)(struct glbSetFile *self, const char *id, size_t *offset, size_t buff_size); 
    int (*addByteCode)(struct glbSetFile *self, const char *bytecode, size_t bytecode_size, size_t *offset);
    int (*addId)(struct glbSetFile *self, const char *id, size_t locset_p, size_t *offset);

} glbSetFile;

extern int glbSetFile_new(struct glbSetFile **self);

