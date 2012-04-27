#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#include "glb_set_file.h"
#include "glb_config.h"

static int
glbSetFile_create_files(struct glbSetFile *self)
{
    int fd;
    char info;

    info = sizeof(size_t);
    
    fd = open(self->path_set, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (fd < 0) return glb_IO_FAIL;
    close(fd);

    fd = open(self->path_locset, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (fd < 0) return glb_IO_FAIL;
    close(fd);
    
    fd = open(self->path_set, O_WRONLY | O_TRUNC | O_CREAT);
    if (fd < 0) return glb_IO_FAIL;
    write(fd, &info, 1);
    close(fd);
    
    fd = open(self->path_locset, O_WRONLY | O_TRUNC | O_CREAT);
    if (fd < 0) return glb_IO_FAIL;
    write(fd, &info, 1);
    close(fd);

    return glb_OK;
}

static int
glbSetFile_buffering(struct glbSetFile *self, size_t offset, char *buffer, size_t size, size_t *result)
{
    int fd;
    size_t res;

    fd = open(self->path_set, O_RDONLY);
    if (fd < 0) {
        close(fd);
        return glb_IO_FAIL;
    }
    
    lseek(fd, offset, SEEK_SET);
    res = read(fd, buffer, size);
    
    *result = res; 
    
    close(fd); 
    return glb_OK;
}

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
    if (GLB_DEBUG_LEVEL_4)
        printf("    ID was not found in set\n");
    return glb_NO_RESULTS;
}

static int
glbSetFile_lookup(struct glbSetFile *self, const char *id, size_t *offset)
{
    int fd;
    int res;
    size_t buff_size;

    if (GLB_DEBUG_LEVEL_4)
        printf("    Lookup in file started.\n");
    
    fd = open(self->path_set, O_RDONLY); 
    if (fd < 0) return glb_IO_FAIL;
    
    lseek(fd, *offset, SEEK_SET);
    buff_size = read(fd, self->buffer, self->buffer_size);
    close(fd);

    if (GLB_DEBUG_LEVEL_4) 
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
    if (!self) return glb_OK;

    if (self->buffer) free(self->buffer);

    free(self);

    return glb_OK;
}

static int
glbSetFile_init(struct glbSetFile *self, const char *name, const char *env_path)
{
    self->init = glbSetFile_init;
    self->del = glbSetFile_del;

    self->addByteCode = glbSetFile_addByteCode;
    self->addId = glbSetFile_addId;
    self->lookup = glbSetFile_lookup;
    self->lookupInBuffer = glbSetFile_lookupInBuffer;
    self->buffering = glbSetFile_buffering;
    self->create_files = glbSetFile_create_files;

    /* 5 means len of .set\0 */
    if (strlen(name)  + 5 > GLB_NAME_LENGTH)
        return glb_FAIL;
    strcpy(self->path_set, name);
    strcat(self->path_set, ".set");

    /* 8 means len of .locset\0 */
    if (strlen(name)  + 8 > GLB_NAME_LENGTH)
        return glb_FAIL;
    strcpy(self->path_locset, name);
    strcat(self->path_locset, ".locset");

    self->create_files(self);

    return glb_OK; 
}

int glbSetFile_new(struct glbSetFile **rec, const char *name, const char *env_path)
{
    struct glbSetFile *self;
    size_t size;
    int result;

    self = malloc(sizeof(struct glbSetFile));
    if (!self) return glb_NOMEM;
    
    memset(self->path_set, 0, GLB_NAME_LENGTH);
    memset(self->path_locset, 0, GLB_NAME_LENGTH);
    
    self->buffer = NULL;
    self->buffer_size = 0;
    
    size = (GLB_ID_MATRIX_DEPTH * sizeof(char) + GLB_SIZE_OF_OFFSET) * GLB_LEAF_SIZE;

    self->buffer = malloc(size);
    
    if (!self->buffer) return glb_NOMEM;
    self->buffer_size = size;

    result = glbSetFile_init(self, name, env_path);
    if (result) goto error;
    *rec = self;

    return glb_OK;
error:
    glbSetFile_del(self);
    return result;
}
