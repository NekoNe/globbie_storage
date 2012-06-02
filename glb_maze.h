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
 *
 *   ----------
 *   glb_maze.h
 *   Globbie Maze Dictionary
 */

#ifndef GLB_MAZE_H
#define GLB_MAZE_H

#include <libxml/parser.h>

struct glbData;
struct glbSet;

struct glbMazeRef
{
    struct glbMazeItem *item;
    struct glbMazeRef *next;
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

    struct glbSet *set;
    const char *set_path;

    size_t num_objs;
    size_t weight;

    struct glbMazeItem *next;
};

struct glbMaze
{
    int id;

    char *path;
    size_t path_size;

    struct glbMazeItem *item_storage;
    struct glbMazeItem **item_index;
    size_t num_items;
    size_t num_root_items;
    size_t item_storage_size;

    struct glbMazeSpec *spec_storage;
    size_t num_specs;
    size_t spec_storage_size;

    struct glbSet **cache_set_storage;
    size_t num_cache_sets;
    size_t cache_set_storage_size;

    struct glbSet **agent_set_storage;
    size_t num_agent_sets;
    size_t agent_set_storage_size;

    struct ooDict *item_dict;

    struct ooSet **search_set_pool;
    size_t search_set_pool_size;
    size_t max_set_pool_size;

    /******** public methods ********/
    int (*init)(struct glbMaze *self);
    int (*del)(struct glbMaze *self);
    const char* (*str)(struct glbMaze *self);

    int (*add)(struct glbMaze *self,
	       struct glbMazeSpec *topic,
	       xmlNodePtr input_node,
	       const char *name,
	       const char *obj_id);

    int (*sort)(struct glbMaze *self);

    int (*update)(struct glbMaze *self,
		  struct glbData *data);

    int (*search)(struct glbMaze *self,
		  struct glbData *data);

};

/* constructor */
extern int glbMaze_new(struct glbMaze **self);

#endif /* GLB_MAZE_H */
