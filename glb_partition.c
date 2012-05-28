#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <zmq.h>

#include "glb_config.h"
#include "oodict.h"
#include "glb_partition.h"



static int
glbPartition_del(struct glbPartition *self)
{
    int i;

    if (!self) return glb_OK;
    
    free(self);
    return glb_OK;
}


static int
glbPartition_add(struct glbPartition *self,
		 char *spec,
		 size_t spec_size,
		 void *obj,
		 size_t obj_size,
		 char *text,
		 size_t text_size,
		 const char **obj_id)
{
    char id_buf[GLB_ID_MATRIX_DEPTH + 1];
    char path_buf[GLB_TEMP_BUF_SIZE];
    char *curr_buf = path_buf;

    struct glbObjRecord *rec;
    int i, ret;
    int fd;

    printf(" partition %d activated!\n", self->id);

    if (self->num_objs + 1 >= self->max_num_objs) {
	printf("  -- Maximum number of objects reached!\n");
	return glb_FAIL;
    }

    memcpy(id_buf, self->curr_obj_id, GLB_ID_MATRIX_DEPTH);
    id_buf[GLB_ID_MATRIX_DEPTH] = '\0';
    inc_id(id_buf);

    rec = malloc(sizeof(struct glbObjRecord));
    if (!rec) return glb_NOMEM;

    rec->meta = spec;
    rec->obj = obj;
    rec->text = text;

    sprintf(curr_buf, "%s/part%d", self->path, self->id);
    curr_buf += strlen(path_buf);

    for (i = 0; i < GLB_ID_MATRIX_DEPTH; i++) {
	*curr_buf =  '/';
	curr_buf++;
	*curr_buf = id_buf[i];
	curr_buf++;
    }
    *curr_buf = '\0';

    /* create path to object's folder */
    printf("mkpath %s\n", path_buf);
    ret = glb_mkpath(path_buf, 0777);
    if (ret != glb_OK) return ret;


    /* write metadata */
    glb_write_file((const char*)path_buf, "meta", spec, spec_size);

    /* write textual content */
    glb_write_file((const char*)path_buf, "text", text, text_size);




    self->obj_index->set(self->obj_index, id_buf, rec);

    memcpy(self->curr_obj_id, id_buf, GLB_ID_MATRIX_DEPTH);
    self->num_objs++;

    *obj_id = self->curr_obj_id;

    return glb_OK;
}

static int
glbPartition_init(struct glbPartition *self)
{
    self->del = glbPartition_del;
    self->add = glbPartition_add;

    return glb_OK;
}

int glbPartition_new(struct glbPartition **rec)
{
    struct glbPartition *self;
    int ret;
    size_t i;

    self = malloc(sizeof(struct glbPartition));
    if (!self) return glb_NOMEM;

    memset(self, 0, sizeof(struct glbPartition));

    self->path = "storage";

    self->max_num_objs = GLB_RADIX_BASE;
    for (i = 0; i < GLB_ID_MATRIX_DEPTH - 1; i++) {
	self->max_num_objs *= GLB_RADIX_BASE;
    }


    ret = ooDict_new(&self->obj_index, GLB_LARGE_DICT_SIZE);
    if (ret != oo_OK) {
	glbPartition_del(self);
	return ret;
    }

    self->curr_obj_id = malloc((GLB_ID_MATRIX_DEPTH + 1) * sizeof(char));
    if (!self->curr_obj_id) {
	glbPartition_del(self);
	return glb_NOMEM;
    }
    memset(self->curr_obj_id, '0', GLB_ID_MATRIX_DEPTH);
    self->curr_obj_id[GLB_ID_MATRIX_DEPTH] = '\0';

    glbPartition_init(self); 

    *rec = self;
    return glb_OK;
}
