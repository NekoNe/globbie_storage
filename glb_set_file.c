#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#include "glb_set_file.h"
#include "glb_config.h"
#include "glb_utils.h"

static int
glbSetFile_open_file(struct glbSetFile *self)
{
    int fd;
    char info;

    info = sizeof(size_t);

    fd = open(self->filename, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (fd < 0) return glb_IO_FAIL;
    close(fd);

    /*fd = open(self->path_locset, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (fd < 0) return glb_IO_FAIL;
    close(fd);
    */
 
    fd = open(self->filename, O_WRONLY | O_TRUNC | O_CREAT);
    if (fd < 0) return glb_IO_FAIL;
    write(fd, &info, 1);
    close(fd);
    
/*    fd = open(self->path_locset, O_WRONLY | O_TRUNC | O_CREAT);
    if (fd < 0) return glb_IO_FAIL;
    write(fd, &info, 1);
    close(fd);
*/

    return glb_OK;
}

static int
glbSetFile_read_buf(struct glbSetFile *self, 
		    size_t offset, 
		    char *buffer, 
		    size_t size, 
		    size_t *result)
{
    int fd;
    size_t res;

    fd = open(self->filename, O_RDONLY);
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
glbSetFile_buf_lookup(struct glbSetFile *self, 
		      const char *id, 
		      size_t buf_size)
{
    size_t i;
    int res;
    const char *cur_id = NULL;

    for (i = 0; i < buf_size; i += GLB_ID_MATRIX_DEPTH) {
	cur_id = self->buffer + i;
	if (!strcmp(id, cur_id)) return glb_OK;
    }

    if (GLB_DEBUG_LEVEL_4)
        printf("    ID was not found in set\n");

    return glb_NO_RESULTS;
}

static int
glbSetFile_lookup(struct glbSetFile *self, const char *id, size_t offset)
{
    int fd;
    int res;
    size_t buf_size;

    if (GLB_DEBUG_LEVEL_4)
        printf("    Looking up obj_id \"%s\" in the set file...\n", id);
    
    fd = open(self->filename, O_RDONLY); 
    if (fd < 0) return glb_IO_FAIL;
    
    lseek(fd, offset, SEEK_SET);
    buf_size = read(fd, self->buffer, self->buffer_size);
    close(fd);

    if (GLB_DEBUG_LEVEL_4) 
        printf("    Buffer was read from file. Buffer size: %d\n", buf_size);

    return  glbSetFile_buf_lookup(self, id, buf_size);
}

static int
glbSetFile_add(struct glbSetFile *self, 
	       const char *id,
	       size_t *offset)
{
    struct stat file_info;
    int rec;
    int fd;
    
    printf("write to file \"%s\"...\n", self->filename);

    fd = open(self->filename, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0) return glb_IO_FAIL;
    
    fstat(fd, &file_info);
    
    write(fd, id, GLB_ID_MATRIX_DEPTH);

    close(fd);

    *offset = file_info.st_size;  
    
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
glbSetFile_init(struct glbSetFile *self, 
		const char *path, 
		const char *name)
{
    char buf[GLB_TEMP_BUF_SIZE];
    int ret;

    printf("set file init...\n");

    self->path = path;
    self->name = name;

    if (self->filename)
	free(self->filename);

    ret = glb_mkpath(path, 0777);
    if (ret != glb_OK) return ret;

    sprintf(buf, "%s/%s.set", path, name);
    self->filename = strdup(buf);
    if (!self->filename) return glb_NOMEM;

    /*ret = glbSetFile_open_file(self);
      if (ret != glb_OK) return ret;*/


    return glb_OK; 
}


int glbSetFile_new(struct glbSetFile **rec)
{
    struct glbSetFile *self;
    size_t size;
    int result;

    self = malloc(sizeof(struct glbSetFile));
    if (!self) return glb_NOMEM;
    
    memset(self, 0, sizeof(struct glbSetFile));
    
    size = (GLB_ID_MATRIX_DEPTH * sizeof(char) + 1) * GLB_LEAF_SIZE;

    self->buffer = malloc(size);
    if (!self->buffer) return glb_NOMEM;
    self->buffer_size = size;

    self->init = glbSetFile_init;
    self->del = glbSetFile_del;

    self->add = glbSetFile_add;
    self->lookup = glbSetFile_lookup;

    self->read_buf = glbSetFile_read_buf;


    *rec = self;

    return glb_OK;
error:
    glbSetFile_del(self);
    return result;
}
