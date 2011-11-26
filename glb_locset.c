/**
 *   Copyright (c) 2011 by Dmitri Dmitriev
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
 *
 *   --------------
 *   glb_locset.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "glb_config.h"
#include "glb_locset.h"

enum ThingFlags {
  ThingMask  = 0x0000,
  ThingFlag0 = 1 << 0,
  ThingFlag1 = 1 << 1,
  ThingError = 1 << 8,
};

/*thingstate |= ThingFlag1;
thingstate &= ~ThingFlag0;
if (thing | ThingError) {...}
*/

static int 
glbLocSet_del(struct glbLocSet *self)
{
    size_t i;
    for (i = 0; i < self->num_children; i++)
	self->children[i]->del(self->children[i]);

    free(self);
    return glb_OK;
}


static const char* 
glbLocSet_str(struct glbLocSet *self, size_t depth)
{
    struct glbLocSet *locset;
    int i;
    size_t offset_size = sizeof(char) * GLB_TREE_OFFSET_SIZE * depth;
    char *offset = malloc(offset_size + 1);
    memset(offset, SPACE_CHAR, offset_size);
    offset[offset_size] = '\0';

    printf("%s(%s",
	   offset, self->name);

    if (self->num_children == 0) {
	printf(")\n");
	return "";
    }
    else 
	printf("\n");

    for (i = 0; i < self->num_children; i++) {
	locset = self->children[i]; 
	locset->str(locset, depth+1);
    }

    printf("%s)\n", offset);

    return "";
}


static int 
glbLocSet_pack(struct glbLocSet *self)
{
    struct glbLocSet *locset;
    int ret;





    return glb_OK;
}

static int 
glbLocSet_read_node(struct glbLocSet *self, 
		   const char **input,
		   struct glbLocSet **child)
{
    struct glbLocSet *locset;
    struct glbLocSet **children;
    const char *c = *input;
    char buf[TMP_ID_BUF_SIZE];
    size_t buf_size = 0;
    bool foreign_symbol_met = false;
    int ret;

    while (*c) {

	if (buf_size >= TMP_ID_BUF_SIZE) return glb_FAIL;

	switch (*c) {
	case '.':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	    buf[buf_size] = *c;
	    buf_size++;
	    break;
	case ' ':
	case '\n':
	case '\r':
	case '\t':
	    break;
	default:
	    foreign_symbol_met = true;
	    break;
	}

	if (foreign_symbol_met) break;

	c++;
    }

    /* no valid id was found */
    if (buf_size == 0) return glb_FAIL;

    /* id is OK, let's create a child node */
    buf[buf_size] = '\0';

    ret = glbLocSet_new(&locset);
    if (ret != glb_OK) return ret;

    locset->name = malloc(buf_size + 1);
    if (!locset->name) return glb_NOMEM;

    strcpy(locset->name, buf);

    /*printf("parent: %s   new child: %s\n", self->name, buf);*/

    children = realloc(self->children, self->num_children + 1);
    if (!children) return glb_NOMEM;

    children[self->num_children] = locset;
    self->children = children;
    self->num_children++;

    locset->parent = self;

    *child = locset;

    /* last symbol must be returned as unread, hence -1 */
    (*input) += buf_size - 1;
 
    return glb_OK;
}



static int 
glbLocSet_import(struct glbLocSet *self, 
		 const char *input)
{
    struct glbLocSet *locset = self, *child;
    const char *c;
    int ret = glb_FAIL;

    if (!input) return ret;
    c = input;

    /* walking over the input string 
     */
    while (*c) {
	
	/*printf("symbol from stream: \"%c\"\n", *c);*/
	
	switch (*c) {
	case '(':
	    /* current locset must be complete */
	    if (!locset->name) goto error;
	    c++;
	    ret = glbLocSet_read_node(locset, &c, &child);
	    if (ret != glb_OK) goto error;
	    locset = child;
	    break;
	case ')':
	    /* close div, rewind */
	    if (locset->parent)
		locset = locset->parent;
	    break;
	case ' ':
	case '\n':
	case '\r':
	case '\t':
	default:
	    break;
	}
      
      c++;
    }

    return glb_OK;

error:

    return ret;
}

/**
 *  glbLocSet initializer
 */
static int 
glbLocSet_init(struct glbLocSet *self)
{
    self->type = GLB_LOC_NUMERIC;
    self->rank = GLB_LOC_ROOT;

    self->name = NULL;

    self->parent = NULL;
    self->children = NULL;
    self->num_children = 0;

    self->bytecode = NULL;
    self->bytecode_size = 0;

    self->bytecode_idx = NULL;
    self->bytecode_idx_size = 0;

    /* set public methods */
    self->init = glbLocSet_init;
    self->del = glbLocSet_del;
    self->str = glbLocSet_str;

    self->import = glbLocSet_import;
    self->pack = glbLocSet_pack;

    return glb_OK;
}

/**
 *  glbLocSet allocator 
 */
int glbLocSet_new(struct glbLocSet **rec)
{
    int ret;
    struct glbLocSet *self = malloc(sizeof(struct glbLocSet));
    if (!self) return glb_NOMEM;

    glbLocSet_init(self);

    *rec = self;
    return glb_OK;
}
