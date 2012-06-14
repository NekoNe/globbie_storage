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
struct glbPartition;

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

    struct glbSetRef *refs;
    size_t num_refs;

    struct glbSet *set;
    const char *set_path;

    size_t num_objs;

    size_t num_requests;

    struct glbMazeItem *next;
};

struct glbMaze
{
    int id;

    char *path;
    size_t path_size;

    const char *storage_path;
    struct glbPartition *partition;

    struct ooDict *item_dict;

    struct glbMazeItem *item_storage;
    struct glbMazeItem **item_index;
    size_t num_items;
    size_t num_root_items;
    size_t item_storage_size;

    struct glbMazeSpec *spec_storage;
    size_t num_specs;
    size_t spec_storage_size;

    /* original text buffer */
    char *text;
    size_t text_size;

    /* results buf */
    char *results;
    char *curr_results_buf;
    size_t results_size;

    /* agent locsets */
    size_t **locsets;
    size_t *num_locs;
    size_t *result_ranges;

    struct glbSet **agent_set_storage;
    size_t num_agents;
    size_t max_agents;

    struct glbSetRef *setref_storage;
    size_t num_setrefs;
    size_t max_setrefs;

   /* set cache */
    struct glbSet *head;
    struct glbSet *tail;
    size_t cache_set_storage_size;

    struct glbSet **search_set_pool;
    size_t search_set_pool_size;
    size_t max_search_set_pool_size;


    /******** public methods ********/
    int (*init)(struct glbMaze *self);
    int (*del)(struct glbMaze *self);
    const char* (*str)(struct glbMaze *self);

    int (*update)(struct glbMaze *self,
		  struct glbData *data);

    int (*sort)(struct glbMaze *self);

    int (*search)(struct glbMaze *self,
		  struct glbData *data);

};

/* constructor */
extern int glbMaze_new(struct glbMaze **self);

#endif /* GLB_MAZE_H */
