/**
 *   Copyright (c) 2011-2012 by Dmitri Dmitriev
 *   All rights reserved.
 *
 *   This file is part of the Globbie Search Engine, 
 *   and as such it is subject to the license stated
 *   in the LICENSE file which you have received 
 *   as part of this distribution.
 *
 *   Project homepage:
 *   <http://www.globbie.net>
 *
 *   Initial author and maintainer:
 *         Dmitri Dmitriev aka M0nsteR <dmitri@globbie.net>

 *   ----------
 *   glb_maze.c
 *   Globbie Maze Dictionary implementation
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/parser.h>

#include "glb_config.h"
#include "glb_maze.h"
#include "glb_set.h"
#include "glb_utils.h"
#include "glb_request_handler.h"
#include "oodict.h"

#define DEBUG_MAZE_LEVEL_1 1
#define DEBUG_MAZE_LEVEL_2 1
#define DEBUG_MAZE_LEVEL_3 1
#define DEBUG_MAZE_LEVEL_4 1

static int 
glbMaze_item_cmp(const void *a,
		const void *b)
{
    struct glbMazeItem **item1, **item2;

    item1 = (struct glbMazeItem**)a;
    item2 = (struct glbMazeItem**)b;

    /*printf("%d %d?\n", (*sol1)->weight, (*sol2)->weight);*/

    /*if ((*item1)->num_locs == (*item2)->num_locs) return 0;*/

    /* ascending order */

/*    if ((*item1)->num_locs > (*item2)->num_locs) return 1;*/

    return -1;
}

static int 
glbSet_cmp(const void *a,
	   const void *b)
{
    struct glbSet **set1, **set2;

    set1 = (struct glbSet**)a;
    set2 = (struct glbSet**)b;

    if ((*set1)->num_objs == (*set2)->num_objs) return 0;

    /* ascending order */
    if ((*set1)->num_objs > (*set2)->num_objs) return 1;

    return -1;
}

static int
glbMaze_show_item(struct glbMaze *self, 
		 struct glbMazeItem *item,
		 size_t depth)
{
    struct glbMazeLoc *loc;
    struct glbMazeSpec *spec;
    struct glbMazeItem *curr_item;

    size_t offset_size = sizeof(char) * OFFSET_SIZE * depth;
    char *offset = malloc(offset_size + 1);

    memset(offset, SPACE_CHAR, offset_size);
    offset[offset_size] = '\0';

    if (depth == 0) puts("-----");

    printf("%sconc: %s   ", offset, item->name);

    if (item->specs) {
	printf("\n%sspecs:\n", offset);

	spec = item->specs;
	while (spec) {

	    printf("%s  spec \"%s\":\n", offset, spec->name);
	    
	    curr_item = spec->items;
	    while (curr_item) {
		glbMaze_show_item(self, curr_item, depth + 1);
		curr_item = curr_item->next;
	    }

	    spec = spec->next;
	}

    }

    free(offset);

    return glb_OK;
}

static const char*
glbMaze_str(struct glbMaze *self)
{
    struct glbMazeItem *item;
    const char *key = NULL;
    void *value = NULL;
    int i;

 
    printf(" *** MAZE ***\n");

    for (i = 0; i < self->num_root_items; i++) {
	item = self->item_index[i];
	glbMaze_show_item(self, item, 0);
    }

    return "";
}

static int 
glbMaze_del(struct glbMaze *self)
{
    if (self->path)
	free(self->path);

    if (self->item_dict) 
	self->item_dict->del(self->item_dict);
	
    if (self->item_storage)
	free(self->item_storage);

    if (self->item_index)
	free(self->item_index);

    if (self->spec_storage)
	free(self->spec_storage);

    if (self->agent_set_storage)
	free(self->agent_set_storage);

    free(self);

    return glb_OK;
}

/* allocate set */
static struct glbSet*
glbMaze_alloc_agent_set(struct glbMaze *self)
{
    struct glbSet *set;

    if (self->num_agent_sets == self->agent_set_storage_size) {
	fprintf(stderr, "Maze's agent set storage capacity exceeded... Resize?\n");
	return NULL;
    }

    set = self->agent_set_storage[self->num_agent_sets];

    self->num_agent_sets++;

    return set;
}

/* allocate item */
static struct glbMazeItem*
glbMaze_alloc_item(struct glbMaze *self)
{
    struct glbMazeItem *item;

    if (self->num_items == self->item_storage_size) {
	fprintf(stderr, "Maze's item storage capacity exceeded... Resize?\n");
	return NULL;
    }

    item = &self->item_storage[self->num_items];

    /* init null values */
    memset(item, 0, sizeof(struct glbMazeItem));

    self->num_items++;

    return item;
} 

/* allocate spec */
static struct glbMazeSpec*
glbMaze_alloc_spec(struct glbMaze *self)
{
    struct glbMazeSpec *spec;

    if (self->num_specs == self->spec_storage_size) {
	fprintf(stderr, "Maze's spec storage capacity exceeded... Resize?\n");
	return NULL;
    }

    spec = &self->spec_storage[self->num_specs];

    /* init null values */
    memset(spec, 0, sizeof(struct glbMazeSpec));

    self->num_specs++;

    return spec;
} 

static struct glbMazeItem*
glbMaze_find_item(struct glbMazeItem *item,
		 const char *name)
{

    while (item) {
	if (!strcmp(item->name, name))
	    return item;
	item = item->next;
    }

    return NULL;
}

static struct glbMazeSpec*
glbMaze_find_spec(struct glbMazeSpec *spec,
		 const char *name)
{
    while (spec) {
	if (!strcmp(spec->name, name))
	    return spec;
	spec = spec->next;
    }

    return NULL;
}



static int 
glbMaze_add_spec(struct glbMaze *self,
		struct glbMazeSpec *topic_spec,
		struct glbMazeItem *complex)
{
    struct glbMazeItem *item;
    struct glbMazeSpec *spec;
    struct glbMazeLoc *loc;

    const char *name;
    int ret;

    name =  (const char*)complex->name;

    printf("  add complex \"%s\" as spec \"%s\"..\n", 
	   name, topic_spec->name);

    item = NULL;

    if (topic_spec->items)
	item = glbMaze_find_item(topic_spec->items, name);


    if (!item) {
	item = glbMaze_alloc_item(self);
	if (!item) return glb_NOMEM;
	item->name = name;
	item->next = topic_spec->items;
	topic_spec->items = item;
    }
    else {
	printf("  spec already exists: %p!\n", item);
    }

    /*printf(" add a loc to spec item...\n");*/


    /* more specs */
    /*if (complex->spec.complex) {
	spec = NULL;
	if (item->specs)
	    spec = glbMaze_find_spec(item->specs, complex->spec.name);

	if (!spec) {
	    spec = self->alloc_spec(self);
	    if (!spec) return glb_NOMEM;
	    spec->name = complex->spec.name;
	    spec->next = item->specs;
	    item->specs = spec;
	}

	ret = glbMaze_add_spec(self, spec, complex->spec.complex);
   

	}*/

    return glb_OK;
}

static int 
glbMaze_add(struct glbMaze *self,
	    struct glbMazeSpec *topic_spec,
	    xmlNodePtr input_node,
	    const char *name,
	    const char *obj_id)
{
    struct glbMazeItem *item;
    struct glbMazeSpec *spec;
    struct glbMazeLoc *loc;

    struct glbSet *set;

    size_t global_offset;

    char buf[GLB_TEMP_BUF_SIZE];
    char *value;
    size_t name_size;
    int ret;

    name_size = strlen(name);

    /* registration of a new root item */
    item = self->item_dict->get(self->item_dict, name);
    if (!item) {
	item = glbMaze_alloc_item(self);
	if (!item) return glb_NOMEM;

	item->name = strdup(name);
	item->name_size = name_size;

	ret = self->item_dict->set(self->item_dict,
				   name, item);

	self->item_index[self->num_root_items] = item;
	self->num_root_items++;

	if (DEBUG_MAZE_LEVEL_3)
	    printf("  ++ root registration of item %s\n", 
		   item->name);
    }

    /* add obj_id to appropriate set */

    /* TODO: set already in cache? */

    /* take one of the agents */
    set = glbMaze_alloc_agent_set(self);
    if (!set) return glb_NOMEM;

    if (DEBUG_MAZE_LEVEL_1)
	printf("   %s : Agent Set ready: %p\n", 
	       self->path, set->init);

    ret = set->init(set,
		    (const char*)self->path, 
		    (const char*)item->name);

    printf("   set file init: %d\n", ret);

    if (ret != glb_OK) return ret;

    if (DEBUG_MAZE_LEVEL_1)
	printf(" add obj_id %s to %s/%s...\n", 
	       obj_id, self->path, item->name );

    ret = set->add(set, obj_id);
    if (ret != glb_OK) return ret;

    item->num_objs++;

    return glb_OK;
}


static int
glbMaze_sort_items(struct glbMaze *self)
{
    if (self->num_root_items == 0) return oo_FAIL;

    /* sorting */
    qsort(self->item_index, 
	  self->num_root_items, 
	  sizeof(struct glbMazeItem*), glbMaze_item_cmp);

    return glb_OK;
}

static int 
glbMaze_add_search_term(struct glbMaze *self, 
			const char *name)
{
    struct glbMazeItem *item;
    struct glbSet *set;
    int ret;

    if (DEBUG_MAZE_LEVEL_2)
	printf("\n  .. add search term %s\n", name);

    item = self->item_dict->get(self->item_dict, name);
    if (!item) {
	if (DEBUG_MAZE_LEVEL_2)
	    printf("\n  .. no such set found: %s\n", name);

	return glb_NO_RESULTS;
    }




    /* take one of the agents */
    set = glbMaze_alloc_agent_set(self);
    if (!set) return glb_NOMEM;

    ret = set->init(set,
		    (const char*)self->path, 
		    (const char*)name);
    if (ret != glb_OK) return ret;

    set->num_objs = item->num_objs;
    item->num_requests++;

    self->search_set_pool[self->search_set_pool_size] = set;
    self->search_set_pool_size++;
  
    return glb_OK;
}

static int
glbMaze_search(struct glbMaze *self, 
	       struct glbData *data)
{
    xmlDocPtr doc;
    xmlNodePtr root, cur_node;
    struct glbRequestHandler *request;
    char *value = NULL;
    int ret = glb_OK;
    char buf[GLB_ID_MATRIX_DEPTH + 1];
    int i;

    if (DEBUG_MAZE_LEVEL_1)
	printf("    !! Maze %d  search...\n", 
	       self->id);

    doc = xmlReadMemory(data->interp, 
			data->interp_size,
			"none.xml", NULL, 0);
    if (!doc) {
        fprintf(stderr, "Failed to parse interp...");
	return glb_FAIL;
    }

    if (DEBUG_MAZE_LEVEL_2)
	printf("    ++ Maze %d: interp XML parse OK!\n", 
	       self->id);

    root = xmlDocGetRootElement(doc);
    if (!root) {
	fprintf(stderr,"empty document\n");
	ret = glb_FAIL;
	goto error;
    }

    if (xmlStrcmp(root->name, (const xmlChar *) "concepts")) {
	fprintf(stderr,"Document of the wrong type: the root node " 
		" must be \"concepts\"");
	ret = glb_FAIL;
	goto error;
    }

    /* reset agents */
    self->num_agent_sets = 0;
    self->search_set_pool_size = 0;

    for (cur_node = root->children; cur_node; cur_node = cur_node->next) {
        if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"conc"))) {

	    value = (char *)xmlGetProp(cur_node,  (const xmlChar *)"n");
	    if (!value) return glb_FAIL;

	    ret = glbMaze_add_search_term(self, value);
	    if (ret != glb_OK) goto error;

	}
    }

    printf("NUM SETS: %d\n", self->search_set_pool_size);

    if (self->search_set_pool_size == 0) return glb_NO_RESULTS;

    /* sorting the sets by size */
    qsort(self->search_set_pool, 
	  self->search_set_pool_size, 
	  sizeof(struct glbSet*), glbSet_cmp);

    ret = glbRequestHandler_new(&request, 
				GLB_RESULT_BATCH_SIZE,
				0, 
				self->search_set_pool,
				self->search_set_pool_size);
    if (ret != glb_OK) return ret;

    printf("  intersection started...\n");

    request->intersect(request);

    printf("NUM RESULTS: %d\n", request->result->answer_actual_size);


    /* for (i = 0; i <  request->result->answer_actual_size; i++) {
	memcpy(buf, request->result->ids[GLB_ID_MATRIX_DEPTH * i], GLB_ID_MATRIX_DEPTH);
	buf[GLB_ID_MATRIX_DEPTH] = '\0';
	printf("%s\n", buf);
	}*/
    request->result->ids[GLB_ID_MATRIX_DEPTH * request->result->answer_actual_size] = '\0';
    printf("%s\n", request->result->ids);

    /* для каждого id читаем его текст и индекс */
    
          /* в индексе находим все концепты и извлекаем списки линейных позиций */

           /* функция формирования фрагмента с подсветкой */



 error:

    if (value)
	xmlFree(value);

    xmlFreeDoc(doc);

    return ret;
}


static int 
glbMaze_update(struct glbMaze *self, 
	       struct glbData *data)
{
    xmlDocPtr doc;
    xmlNodePtr root, cur_node;
    char *value = NULL;
    int ret = glb_OK;

    if (DEBUG_MAZE_LEVEL_1)
	printf("    !! Maze %d  got obj: %s\n", 
	       self->id, data->local_id);

    doc = xmlReadMemory(data->index, 
			data->index_size,
			"none.xml", NULL, 0);
    if (!doc) {
        fprintf(stderr, "Failed to parse document: \"%s\"", data->local_id);
	return glb_FAIL;
    }

    if (DEBUG_MAZE_LEVEL_2)
	printf("    ++ Maze %d: \"%s\"XML parse OK!\n", 
	       self->id, data->local_id);

    root = xmlDocGetRootElement(doc);
    if (!root) {
	fprintf(stderr,"empty document\n");
	ret = glb_FAIL;
	goto error;
    }

    if (xmlStrcmp(root->name, (const xmlChar *) "maze")) {
	fprintf(stderr,"Document of the wrong type: the root node " 
		" must be \"maze\"");
	ret = glb_FAIL;
	goto error;
    }

    /* reset agents */
    self->num_agent_sets = 0;

    for (cur_node = root->children; cur_node; cur_node = cur_node->next) {
        if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"conc"))) {

	    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"n");
	    if (!value) continue;

	    ret = glbMaze_add(self, 
			      NULL, 
			      cur_node, 
			      (const char*)value,
			      data->local_id);
	    if (ret == glb_NOMEM) goto error;
	}
    }


 error:

    if (value)
	xmlFree(value);

    xmlFreeDoc(doc);

    return ret;
}



static int 
glbMaze_init(struct glbMaze *self)
{

    self->del           = glbMaze_del;
    self->init          = glbMaze_init;
    self->str           = glbMaze_str;

    self->update          = glbMaze_update;
    self->add           = glbMaze_add;
    self->sort          = glbMaze_sort_items;
    self->search          = glbMaze_search;

    return glb_OK;
}

/* constructor */
extern int
glbMaze_new(struct glbMaze **maze)
{
    int i, ret;
    struct glbSet *set;

    struct glbMaze *self = malloc(sizeof(struct glbMaze));
    if (!self) return glb_NOMEM;

    memset(self, 0, sizeof(struct glbMaze));

    self->del = glbMaze_del;

    /* allocate items */
    self->item_storage = malloc(sizeof(struct glbMazeItem) *\
				GLB_MAZE_ITEM_STORAGE_SIZE);
    if (!self->item_storage) {
	self->del(self);
	return glb_NOMEM;
    }

    self->item_index = malloc(sizeof(struct glbMazeItem*) *\
				GLB_MAZE_ITEM_STORAGE_SIZE);
    if (!self->item_index) {
	self->del(self);
	return glb_NOMEM;
    }

    self->item_storage_size = GLB_MAZE_ITEM_STORAGE_SIZE;

    ret = ooDict_new(&self->item_dict, GLB_LARGE_DICT_SIZE);
    if (ret != glb_OK) {
	self->del(self);
	return ret;
    }

    /* allocate specs */
    self->spec_storage = malloc(sizeof(struct glbMazeSpec) *\
				GLB_MAZE_SPEC_STORAGE_SIZE);
    if (!self->spec_storage) {
	self->del(self);
	return glb_NOMEM;
    }
    self->spec_storage_size = GLB_MAZE_SPEC_STORAGE_SIZE;

    /* allocate agent sets */
    self->agent_set_storage = malloc(GLB_MAZE_AGENT_SET_STORAGE_SIZE *\
				     sizeof(struct glbSet*));
    if (!self->agent_set_storage) return glb_NOMEM;

    for (i = 0; i < GLB_MAZE_AGENT_SET_STORAGE_SIZE; i++) {
	ret = glbSet_new(&set);
	if (ret != glb_OK) return glb_NOMEM;
	self->agent_set_storage[i] = set;
    }
    self->agent_set_storage_size = GLB_MAZE_AGENT_SET_STORAGE_SIZE;


    self->search_set_pool = malloc(GLB_MAZE_AGENT_SET_STORAGE_SIZE *\
				     sizeof(struct glbSet*));
    if (!self->search_set_pool) return glb_NOMEM;
    self->max_search_set_pool_size = GLB_MAZE_AGENT_SET_STORAGE_SIZE;


    /* allocate cache sets */
    for (i = 0; i < GLB_MAZE_CACHE_SET_STORAGE_SIZE; i++) {
	ret = glbSet_new(&set);
	if (ret != glb_OK) return glb_NOMEM;

	/* first set */
	if (!self->head) {
	    self->head = set;
	    self->tail = set;
	    continue;
	}
	
	set->next = self->head;
	self->head = set;
    }
    self->cache_set_storage_size = GLB_MAZE_CACHE_SET_STORAGE_SIZE;



    glbMaze_init(self);

    *maze = self;

    return glb_OK;
}
