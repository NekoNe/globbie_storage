#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>


#include "glb_set.h"
#include "glb_config.h"

static int
glbSetFile_lookupInBuffer(struct glbSetFile *self, const char *id, size_t *offset, size_t buff_size)
{
    size_t i;
    int res;
          
    for (i = 0; i < buff_size; i += (GLB_ID_MATRIX_DEPTH + sizeof(size_t))) {
        res = compare(id, self->buffer + i);
        if (res == glb_EQUALS) {
            memcpy(offset, self->buffer + i + GLB_ID_MATRIX_DEPTH, sizeof(size_t));
            return glb_OK;
        }
    }
    if (DEBUG_LEVEL_4)
        printf("    ID was not found in set\n");
    return glb_NO_RESULTS;
}

static int
glbSetFile_lookup(struct glbSetFile *self, const char *id, size_t *offset)
{
    int fd;
    int res;
    size_t buff_size;

    if (DEBUG_LEVEL_4)
        printf("    Lookup in file started.\n");
    
    fd = open(self->path_set, O_RDONLY); 
    if (fd < 0) return glb_IO_FAIL;
    
    lseek(fd, *offset, SEEK_SET);
    buff_size = read(fd, self->buffer, self->buffer_size);
    close(fd);

    if (DEBUG_LEVEL_4) 
        printf("    Buffer was read from file. Buffer size: %d\n", buff_size);
    return  self->lookupInBuffer(self, id, offset, buff_size);
}

static int
glbSetFile_addId(struct glbSetFile *self, const char *id, size_t locset_p, size_t *offset)
{
    struct stat file_info;
    int rec;
    int fd;
    
    fd = open(self->path_set, O_WRONLY | O_APPEND | O_CREAT);
    if (fd < 0) return glb_IO_FAIL;
    
    fstat(fd, &file_info);
    
    write(fd, id, GLB_ID_MATRIX_DEPTH);
    write(fd, &locset_p, sizeof(size_t));

    close(fd);
    *offset = file_info.st_size;  
    
    return glb_OK;
}
    
static int
glbSetFile_addByteCode(struct glbSetFile *self, const char *bytecode, size_t bytecode_size, size_t *offset)
{
    struct stat file_info;
    int rec;
    int fd;

    if ((!bytecode)) /*|| (!bytecode_size)) */ {
        *offset = 0;
        return glb_OK; 
    }

    fd = open(self->path_locset, O_WRONLY | O_APPEND | O_CREAT);
    if (fd < 0) return glb_IO_FAIL;
    
    fstat(fd, &file_info);
    /*TODO: errors watch */
    write(fd, &bytecode_size, sizeof(size_t));
    write(fd, bytecode, bytecode_size);

    *offset = file_info.st_size;
    close(fd);

    return glb_OK;
}

static int 
glbSetFile_del(struct glbSetFile *self)
{
    /* TODO */
    return glb_OK;
}

static int
glbSetFile_init(struct glbSetFile *self)
{
    self->init = glbSetFile_init;
    self->del = glbSetFile_del;

    self->addByteCode = glbSetFile_addByteCode;
    self->addId = glbSetFile_addId;
    self->lookup = glbSetFile_lookup;
    self->lookupInBuffer = glbSetFile_lookupInBuffer;

    return glb_OK; 
}

int glbSetFile_new(struct glbSetFile **rec)
{
    struct glbSetFile *self;

    self = malloc(sizeof(struct glbSetFile));
    if (!self) return glb_NOMEM;
    self->buffer = malloc((GLB_ID_MATRIX_DEPTH * sizeof(char) + sizeof(size_t)) * GLB_LEAF_SIZE);
    if (!self->buffer) return glb_NOMEM;
    self->buffer_size = (GLB_ID_MATRIX_DEPTH * sizeof(char) + sizeof(size_t)) * GLB_LEAF_SIZE; /*do not mul twice */

    glbSetFile_init(self);
    *rec = self;

    return glb_OK;
}
