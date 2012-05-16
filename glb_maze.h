/**
 *   Copyright (c) 2011-2012 by Dmitri Dmitriev
 *   All rights reserved.
 *
 *   This file is part of the OOmnik Conceptual Processor, 
 *   and as such it is subject to the license stated
 *   in the LICENSE file which you have received 
 *   as part of this distribution.
 *
 *   Project homepage:
 *   <http://www.oomnik.ru>
 *
 *   Initial author and maintainer:
 *         Dmitri Dmitriev aka M0nsteR <dmitri@globbie.net>
 *
 *   --------
 *   oomaze.h
 *   OOmnik Maze Dictionary
 */
#ifndef GLB_MAZE_H
#define GLB_MAZE_H

#include "ooconfig.h"

struct glbMazeRef
{
    struct glbMazeItem *item;
    struct glbMazeRef *next;
};

/* original location */
struct glbMazeLoc
{
    size_t linear_begin;
    size_t linear_end;

    struct glbMazeLoc *next;
};

struct glbMazeSpec
{
    const char *name;
    struct glbMazeItem *topic;

    struct glbMazeItem *items;
    struct glbMazeSpec *next;
};

struct glbMazeTopic
{
    const char *name;

    struct glbMazeItem *items;
    struct glbMazeTopic *next;
};


struct glbMazeItem
{
    const char *name;
    size_t name_size;

    struct glbMazeSpec *topics;
    struct glbMazeSpec *specs;

    struct glbMazeItem *subclasses;
    struct glbMazeItem *peers;

    struct glbMazeLoc *locs;
    size_t num_locs;

    struct glbMazeItem *next;
};

struct glbMaze
{
    struct ooAccu *accu;

    struct glbMazeItem *item_storage;
    struct glbMazeItem **item_index;
    size_t num_items;
    size_t num_root_items;
    size_t item_storage_size;

    struct glbMazeSpec *spec_storage;
    size_t num_specs;
    size_t spec_storage_size;

    struct glbMazeLoc *loc_storage;
    size_t num_locs;
    size_t loc_storage_size;

    struct ooDict *item_dict;

    /******** public methods ********/
    int (*init)(struct glbMaze *self);
    int (*del)(struct glbMaze *self);
    const char* (*str)(struct glbMaze *self);

    int (*add)(struct glbMaze *self,
	       struct glbMazeSpec *topic,
	       struct glbMazeItem *item,
	       size_t depth);

    int (*sort)(struct glbMaze *self);

    int (*present)(struct glbMaze *self,
		   output_type format);

    struct glbMazeItem* (*alloc_item)(struct glbMaze *self);
    struct glbMazeSpec* (*alloc_spec)(struct glbMaze *self);
    struct glbMazeLoc* (*alloc_loc)(struct glbMaze *self);
    struct glbMazeRef* (*alloc_ref)(struct glbMaze *self);
};

/* constructor */
extern int glbMaze_new(struct glbMaze **self);

#endif /* GLB_MAZE_H */
