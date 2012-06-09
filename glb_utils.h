#ifndef GLB_UTILS_H
#define GLB_UTILS_H

#include <stdio.h>
#include <stdlib.h>

#include <libxml/parser.h>

#include "glb_config.h"


#define GLB_MAX(a, b)  (((a) > (b)) ? (a) : (b))

#define GLB_MIN(a, b)  (((a) < (b)) ? (a) : (b))

/* сколько символов в тексте перекрывают оба отрезка. 
 не пересечение, а отрезки + пространство между */
#define GLB_CONTEXT_AREA(b1, e1, b2, e2) (GLB_MAX((e1), (e2))) + 1 - (GLB_MIN((b1), (b2)))

struct glbData {
    char *id;
    size_t id_size;

    char *ticket;
    size_t ticket_size;

    char *local_id;
    size_t local_id_size;

    char *local_path;
    size_t local_path_size;

    char *spec;
    size_t spec_size;

    char *obj;
    size_t obj_size;

    char *text;
    size_t text_size;

    char *interp;
    size_t interp_size;

    char *topics;
    size_t topic_size;

    char *index;
    size_t index_size;

    char *query;
    size_t query_size;

    char *reply;
    size_t reply_size;

    char *metadata;
    size_t metadata_size;

    char *results;
    size_t result_size;
    size_t num_results;

    char *control_msg;
    size_t control_msg_size;

    int (*del)(struct glbData *self);
    int (*reset)(struct glbData *self);

};

extern int glbData_new(struct glbData **self);



int compare(const char *a, const char *b);
int inc_id(char *id);
const char *max_id(const char *a, const char *b);
const char *min_id(const char *a, const char *b);
int mkpath(const char *path, mode_t mode);

int glb_make_id_path(char *buf,
		     const char *path,
		     const char *id, 
		     const char *filename);
extern int 
glb_get_conc_prefix(const char *name,
		    size_t name_size,
		    char *prefix);
extern int
glb_copy_xmlattr(xmlNode    *input_node,
		 const char *attr_name,
		 char       **result,
		 size_t     *result_size);

extern int
glb_get_xmlattr_num(xmlNode *input_node,
		    const char *attr_name,
		    long *result);

extern int 
glb_remove_nonprintables(char *data);

#endif

