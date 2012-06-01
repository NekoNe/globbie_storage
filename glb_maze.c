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
#include "oodict.h"

static int 
glbMaze_item_cmp(const void *a,
		const void *b)
{
    struct glbMazeItem **item1, **item2;

    item1 = (struct glbMazeItem**)a;
    item2 = (struct glbMazeItem**)b;

    /*printf("%d %d?\n", (*sol1)->weight, (*sol2)->weight);*/

    if ((*item1)->num_locs == (*item2)->num_locs) return 0;

    /* ascending order */
    if ((*item1)->num_locs > (*item2)->num_locs) return 1;
    return -1;
}


static int
glbMaze_newSet(struct glbMaze *self, char *name, struct glbSet **answer)
{
    int result;

    struct glbSet *set;
    struct glbSetFile *file;
    struct glbIndexTree *index;

    if (GLB_COLLECTION_DEBUG_LEVEL_2)
        printf("adding new struct glbSet [%s]\n", name);
    
    result = glbSet_new(&set, name, "");
    if (result) return glb_NOMEM;
   
/*
    if (self->set_index->set(self->set_index, name, set)) {
        set->del(set);
        return glb_NOMEM;
    }
*/


    *answer = set;
    return glb_OK;
}

static int
glbMaze_findSet(struct glbMaze *self, char *name, struct glbSet **set)
{

    /* *set = self->set_index->get(self->set_index, name); */

    return glb_OK;
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

    loc = item->locs;
    while (loc) {
	printf(" [%d:%d] ", 
	       loc->linear_begin, loc->linear_end);
	loc = loc->next;
    }
    puts("");
    
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

    if (self->item_dict) 
	self->item_dict->del(self->item_dict);
	
    if (self->item_storage)
	free(self->item_storage);

    if (self->item_index)
	free(self->item_index);

    if (self->spec_storage)
	free(self->spec_storage);

    if (self->loc_storage)
	free(self->loc_storage);

    free(self);

    return glb_OK;
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

/* allocate loc */
static struct glbMazeLoc*
glbMaze_alloc_loc(struct glbMaze *self)
{
    struct glbMazeLoc *loc;

    if (self->num_locs == self->loc_storage_size) {
	fprintf(stderr, "Maze's loc storage capacity exceeded... Resize?\n");
	return NULL;
    }

    loc = &self->loc_storage[self->num_locs];

    /* init null values */
    memset(loc, 0, sizeof(struct glbMazeLoc));

    self->num_locs++;

    return loc;
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
	item = self->alloc_item(self);
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
	    xmlNodePtr input_node)
{
    struct glbMazeItem *item;
    struct glbMazeSpec *spec;
    struct glbMazeLoc *loc;

    size_t global_offset;
    char *name;
    char *value;
    size_t name_size;
    int ret;

    name = (char *)xmlGetProp(input_node,  (const xmlChar *)"n");
    if (!name) return glb_FAIL;

    /*printf(" add conc: %s\n", name); */

    /* link yourself back to the topic */
    /*if (topic_spec) {
	ret = glbMaze_add_spec(self, topic_spec, complex);
	if (ret != glb_OK) return ret;
	}*/

    /* registration of a new root item */
    item = self->item_dict->get(self->item_dict, name);
    if (!item) {
	item = self->alloc_item(self);
	if (!item) return glb_NOMEM;
	item->name = strdup(name);
	item->name_size = name_size;
	ret = self->item_dict->set(self->item_dict,
				   name, item);

	self->item_index[self->num_root_items] = item;
	self->num_root_items++;

	/*printf("  ++ root registration of item %s [%p]\n", 
	  item->name, item); */
    }
    xmlFree(name);


    /* printf("MAZE %d ITEM: %s\n", self->id, item->name); */

    /* add locset */


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
glbMaze_read_XML(struct glbMaze *self, 
		 struct glbData *data)
{
    xmlDocPtr doc;
    xmlNodePtr root, cur_node;
    char *value;
    int ret = glb_OK;

    /*printf("    !! Maze %d  got obj: %s\n", 
      self->id, data->local_id); */

    doc = xmlReadMemory(data->index, 
			data->index_size, 
			"none.xml", NULL, 0);
    if (!doc) {
        fprintf(stderr, "Failed to parse document\n");
	return glb_FAIL;
    }

    /*printf("    ++ Maze %d: XML parse OK!\n", self->id); */

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

    for (cur_node = root->children; cur_node; cur_node = cur_node->next) {
        if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"conc"))) {

	    ret = glbMaze_add(self, NULL, cur_node);
	    if (ret == glb_NOMEM) goto error;

	}
    }



 error:

    xmlFreeDoc(doc);

    return ret;
}

static int 
glbMaze_init(struct glbMaze *self)
{
    self->del           = glbMaze_del;
    self->init          = glbMaze_init;
    self->str           = glbMaze_str;
    self->alloc_item    = glbMaze_alloc_item;
    self->alloc_spec    = glbMaze_alloc_spec;
    self->alloc_loc     = glbMaze_alloc_loc;
    self->add           = glbMaze_add;
    self->sort          = glbMaze_sort_items;
    self->read          = glbMaze_read_XML;

    return glb_OK;
}

/* constructor */
extern int
glbMaze_new(struct glbMaze **maze)
{
    int ret;

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

    /* allocate locs */
    self->loc_storage = malloc(sizeof(struct glbMazeLoc) *\
				GLB_MAZE_LOC_STORAGE_SIZE);
    if (!self->loc_storage) {
	self->del(self);
	return glb_NOMEM;
    }
    self->loc_storage_size = GLB_MAZE_LOC_STORAGE_SIZE;


    glbMaze_init(self);

    *maze = self;

    return glb_OK;
}
