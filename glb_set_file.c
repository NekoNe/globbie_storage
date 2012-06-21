#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#include "glb_set_file.h"
#include "glb_index_tree.h"
#include "glb_config.h"
#include "glb_utils.h"

#define GLB_DEBUG_SETFILE_LEVEL_1 1
#define GLB_DEBUG_SETFILE_LEVEL_2 1
#define GLB_DEBUG_SETFILE_LEVEL_3 0
#define GLB_DEBUG_SETFILE_LEVEL_4 0

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

    if (GLB_DEBUG_SETFILE_LEVEL_3)
	printf("  ..read buf of size %d from file \"%s\" [offset: %d]\n", 
	       size, self->filename, offset);

    fd = open(self->filename, O_RDONLY);
    if (fd < 0) {
        close(fd);
        return glb_IO_FAIL;
    }
    
    lseek(fd, offset, SEEK_SET);
    res = read(fd, buffer, size);
    
    if (GLB_DEBUG_SETFILE_LEVEL_3)
	printf("   ++ actual read: %d\n", res);

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

	res = compare(cur_id, id);
        if (res == glb_EQUALS) return glb_OK;
        if (res == glb_MORE) return glb_NO_RESULTS;
    }

    if (GLB_DEBUG_SETFILE_LEVEL_4)
        printf("    ID was not found in set\n");

    return glb_NO_RESULTS;
}

static int
glbSetFile_lookup(struct glbSetFile *self, 
		  const char *id, 
		  size_t offset)
{
    int fd;
    int res;
    size_t buf_size;

    if (GLB_DEBUG_SETFILE_LEVEL_4)
        printf("    Looking up obj_id \"%s\" in the set file...\n", id);
    
    fd = open(self->filename, O_RDONLY); 
    if (fd < 0) return glb_IO_FAIL;
    
    lseek(fd, offset, SEEK_SET);
    buf_size = read(fd, self->buffer, self->buffer_size);
    close(fd);

    if (GLB_DEBUG_SETFILE_LEVEL_4) 
        printf("    Buffer was read from file. Buffer size: %d\n", buf_size);

    return glbSetFile_buf_lookup(self, id, buf_size);
}

static int
glbSetFile_add(struct glbSetFile *self, 
	       const char *id,
	       size_t *offset)
{
    struct stat file_info;
    int rec;
    int fd;

    if (GLB_DEBUG_SETFILE_LEVEL_3) 
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
glbSetFile_read_index(struct glbSetFile *self, 
		      struct glbIndexTree *index)
{

    char buf[GLB_ID_MATRIX_DEPTH * GLB_LEAF_SIZE];
    const size_t buf_size = GLB_ID_MATRIX_DEPTH * GLB_LEAF_SIZE;
    char id[GLB_ID_MATRIX_DEPTH + 1];
    int fd;
    size_t offset = 0;
    size_t res;
    int ret = glb_OK;
    struct glbIndexTreeNode *node;

    fd = open(self->filename, O_RDONLY);
    if (fd < 0) {
        close(fd);
	/* no index found */
        return glb_OK;
    }
    
    while (res = read(fd, buf, buf_size)) {

	memcpy(id, buf, GLB_ID_MATRIX_DEPTH);

	/* printf("\n   ++ actual batch read: %d bytes from %s...\n", 
	   res, self->filename); */

	ret = index->addElem(index, (const char*)id, offset);
	if (ret != glb_OK) goto error;


	node = &index->array[index->num_nodes - 1];
	node->id_last = node->id;


	offset += res;
    }

    
error:
    close(fd); 

    return ret;
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
		const char *env_path, 
		const char *name,
		size_t name_size)
{
    char prefix[GLB_TEMP_SMALL_BUF_SIZE];
    char buf[GLB_TEMP_BUF_SIZE];
    int ret;

    self->path = env_path;
    self->name = name;

    if (self->filename)
	free(self->filename);

    glb_get_conc_prefix(name, name_size, prefix);

    sprintf(buf, "%s/%s/",
	    env_path, prefix); 

    ret = glb_mkpath(buf, 0777);
    if (ret != glb_OK) return ret;

    sprintf(buf, "%s/%s/%s.set", 
	    env_path, prefix, name); 

    self->filename = strdup(buf);
    if (!self->filename) return glb_NOMEM;

    /*printf("    init set filename: \"%s\"\n", self->filename);*/

    /* TODO check header */
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
    
    size = (GLB_ID_MATRIX_DEPTH * sizeof(char)) * GLB_LEAF_SIZE;

    self->buffer = malloc(size);
    if (!self->buffer) return glb_NOMEM;
    self->buffer_size = size;

    self->init = glbSetFile_init;
    self->del = glbSetFile_del;

    self->add = glbSetFile_add;
    self->lookup = glbSetFile_lookup;

    self->read = glbSetFile_read_index;
    self->read_buf = glbSetFile_read_buf;


    *rec = self;

    return glb_OK;
error:
    glbSetFile_del(self);
    return result;
}
