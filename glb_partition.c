#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <libxml/parser.h>

#include <zmq.h>

#include "glb_config.h"
#include "oodict.h"
#include "glb_utils.h"
#include "glb_partition.h"



static int
glbPartition_del(struct glbPartition *self)
{
    int i;

    if (self->path) free(self->path);

    free(self);
    return glb_OK;
}

static int
glbPartition_create_metadata(struct glbPartition *self,
			     struct glbData *data)
{
    xmlDocPtr doc;
    xmlNodePtr root, cur_node;
    char *value;
    char *name;
    size_t name_size;
    char output_buf[GLB_TEMP_BUF_SIZE];
    char *buf;
    size_t buf_size = 0;
    bool first_occur;

    int i, ret = glb_OK;

    /*printf("    !! Creating metadata of \"%s\"\n", 
	   data->local_id);
	   printf("    Extract global obj id: %s\n", data->spec);*/

    if (!data->spec) return glb_FAIL;

    /* skip over command */
    buf = data->spec + strlen("ADD ");
    data->id = strdup(buf);
    data->id_size = strlen(buf);

    if (!data->topics) return glb_FAIL;
    
    doc = xmlReadMemory(data->topics, 
			data->topic_size, 
			"none.xml", NULL, 0);
    if (!doc) {
        fprintf(stderr, "Failed to parse document\n");
	return glb_FAIL;
    }

    /*printf("    ++  topics XML parse: %s\n", data->topics); */

    root = xmlDocGetRootElement(doc);
    if (!root) {
	fprintf(stderr,"empty document\n");
	ret = glb_FAIL;
	goto error;
    }

    if (!xmlStrcmp(root->name, (const xmlChar *) "notopics")) 
	goto error;

    if (xmlStrcmp(root->name, (const xmlChar *) "topics")) {
	fprintf(stderr,"Document of the wrong type: the root node " 
		" must be \"topics\"");
	ret = glb_FAIL;
	goto error;
    }


    buf = output_buf;
    buf[0] = '\0';

    first_occur = true;
    for (cur_node = root->children; cur_node; cur_node = cur_node->next) {
        if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"topic"))) {
	    name = (char *)xmlGetProp(cur_node,  (const xmlChar *)"name");
	    if (!name) continue;

	    if (first_occur) {
		sprintf(buf, "%s", name);
		first_occur = false;
	    }
	    else
		sprintf(buf, " | %s", name);

            buf_size = strlen(buf);
            buf += buf_size;
	}
    }

    buf_size = strlen(output_buf);
    if (!buf_size) goto error;

    data->metadata = strdup(output_buf);
    data->metadata_size = buf_size;

    /* strip off last '/' */
    /*data->metadata[buf_size - 1] = '\0';*/


error:

    xmlFreeDoc(doc);

    return glb_OK;
}

static int
glbPartition_update_config(struct glbPartition *self)
{
    char buf[GLB_TEMP_BUF_SIZE];
    int ret;

    buf[0] = '\0';
    sprintf(buf, "<partition>\n<state curr_id=\"%s\"/>\n</partition>",
	self->curr_obj_id);

    ret = glb_write_file((const char*)self->path, 
			 "config.ini", buf, strlen(buf));

    return glb_OK;
}


static int
glbPartition_add(struct glbPartition *self,
		 struct glbData *data)
{
    char id_buf[GLB_ID_MATRIX_DEPTH + 1];
    char path_buf[GLB_TEMP_BUF_SIZE];
    char *curr_buf = path_buf;
    size_t path_size = 0;

    struct glbObjRecord *rec;
    int i, ret;
    int fd;

    printf("    !! Storage Partition #%d activated!\n", self->id);

    if (self->num_objs + 1 >= self->max_num_objs) {
	printf("    -- NB: Maximum number of objects reached!\n");
	return glb_FAIL;
    }

    memcpy(id_buf, self->curr_obj_id, GLB_ID_MATRIX_DEPTH);
    id_buf[GLB_ID_MATRIX_DEPTH] = '\0';
    inc_id(id_buf);

    printf("    == CURR OBJ ID: %s\n", id_buf);

    curr_buf[0] = '\0';

    sprintf(curr_buf, "%s", self->path);
    path_size = strlen(curr_buf);
    curr_buf += path_size;


    /* treat each digit in the id as a folder */
    for (i = 0; i < GLB_ID_MATRIX_DEPTH; i++) {
	*curr_buf =  '/';
	curr_buf++;
	*curr_buf = id_buf[i];
	curr_buf++;
    }
    *curr_buf = '\0';

    /* create path to object's folder */
    /*printf("mkpath %s\n", path_buf);*/

    ret = glb_mkpath(path_buf, 0777);
    if (ret != glb_OK) return ret;


    /* write metadata */
    glb_write_file((const char*)path_buf, 
		   "meta", data->spec, data->spec_size);

    /* write textual content */
    /*glb_remove_nonprintables(path_buf);*/
    glb_write_file((const char*)path_buf, 
		   "text", data->text, data->text_size);

    /* write interp */
    glb_write_file((const char*)path_buf, 
		   "interp.sem", data->interp, data->interp_size);

    /* write topics */
    glb_write_file((const char*)path_buf, 
		   "topics", data->topics, data->topic_size);

    /* write index */
    glb_write_file((const char*)path_buf, 
		   "index.db", data->index, data->index_size);

    printf("    !! Storage Partition #%d:  database files written: %s\n", 
	   self->id, (const char*)path_buf);

    /* partition index of objects */
    /*self->obj_index->set(self->obj_index, id_buf, rec);*/

    memcpy(self->curr_obj_id, id_buf, GLB_ID_MATRIX_DEPTH);
    self->num_objs++;

    glbPartition_update_config(self);

    data->local_id = strdup(self->curr_obj_id);
    data->local_id_size = GLB_ID_MATRIX_DEPTH;

    data->local_path = strdup(path_buf);
    data->local_path_size = path_size;

    ret = glbPartition_create_metadata(self, data);

    if (!data->metadata) {
	data->metadata = strdup(GENERIC_TOPIC_NAME);
	data->metadata_size = strlen(GENERIC_TOPIC_NAME);
    }

    return glb_OK;
}


static int
glbPartition_read_config(struct glbPartition *self)
{
    char buf[GLB_TEMP_BUF_SIZE];
    xmlDocPtr doc;
    xmlNodePtr root, cur_node;
    char *value;
    int ret;

    buf[0] = '\0';
    sprintf(buf, "%s/config.ini", self->path);

    doc = xmlParseFile((const char*)buf);
    if (!doc) {
	fprintf(stderr, "\n    -- No prior config file found."
                        " Fresh start!\n", buf);
	ret = -1;
	goto error;
    }

    root = xmlDocGetRootElement(doc);
    if (!root) {
	fprintf(stderr,"empty document\n");
	ret = -2;
	goto error;
    }

    if (xmlStrcmp(root->name, (const xmlChar *) "partition")) {
	fprintf(stderr,"Document of the wrong type: the root node " 
		" must be \"partition\"");
	ret = -3;
	goto error;
    }

    for (cur_node = root->children; cur_node; cur_node = cur_node->next) {
        if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"state"))) {
	    value = (char *)xmlGetProp(cur_node,  (const xmlChar *)"curr_id");
	    if (value) {
		memcpy(self->curr_obj_id, value, GLB_ID_MATRIX_DEPTH);

		xmlFree(value);
	    }
	}
    }

error:
    xmlFreeDoc(doc);

    return glb_OK;
}

static int
glbPartition_init(struct glbPartition *self)
{
    char buf[GLB_TEMP_BUF_SIZE];
    int ret;

    if (!self->env_path) return glb_FAIL;

    buf[0] = '\0';
    sprintf(buf, "%s/part%d", self->env_path, self->id);

    self->path = strdup(buf);

    ret = glb_mkpath(self->path, 0777);
    if (ret != glb_OK) return ret;

    printf("    .. Partition #%d: %s reading config...\n",
	   self->id, self->path);

    ret = glbPartition_read_config(self);

    printf("    !! curr_id: %s\n", self->curr_obj_id);

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

    self->max_num_objs = GLB_RADIX_BASE;
    for (i = 0; i < GLB_ID_MATRIX_DEPTH - 1; i++) {
	self->max_num_objs *= GLB_RADIX_BASE;
    }


    ret = ooDict_new(&self->obj_index, GLB_MEDIUM_DICT_SIZE);
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

    self->del = glbPartition_del;
    self->add = glbPartition_add;
    self->init = glbPartition_init;

    *rec = self;
    return glb_OK;
}
