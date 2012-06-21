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

typedef enum output_dest_t { GLB_SEARCH_RESULTS, 
			     GLB_DOMAINS, 
			     GLB_METADATA, 
			     GLB_TOPICS } output_dest_t;

struct glbMazeRef
{
    struct glbMazeItem *item;
    size_t num_objs;
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
    char *name;
    size_t name_size;

    struct glbMazeSpec *topics;
    struct glbMazeSpec *specs;

    struct glbMazeRef *refs;
    size_t num_refs;

    /*struct glbSet *set;
    const char *set_path;*/

    /* ids cache buffer */
    char *obj_ids;
    size_t max_obj_ids;
    size_t num_obj_ids;

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
    size_t max_results_size;
    size_t results_free_space;

    /* domain buf */
    char *domains;
    char *curr_domains_buf;
    size_t domains_size;
    size_t max_domains_size;
    size_t domains_free_space;



    /* agent locsets */
    size_t **locsets;
    size_t *num_locs;
    size_t *result_ranges;

    struct glbSet **agent_set_storage;
    size_t num_agents;
    size_t max_agents;

    struct glbMazeRef *ref_storage;
    size_t num_refs;
    size_t max_refs;

   /* set cache */
    struct glbSet *head;
    struct glbSet *tail;
    size_t cache_set_storage_size;

    struct glbSet **search_set_pool;
    size_t search_set_pool_size;
    size_t max_search_set_pool_size;

  
    struct glbRequestHandler *request;

    /******** public methods ********/
    int (*init)(struct glbMaze *self);
    int (*del)(struct glbMaze *self);
    const char* (*str)(struct glbMaze *self);

    int (*write)(struct glbMaze *self,
		 output_dest_t dest,
		 const char    *buf,
		 size_t        buf_size);

    int (*read_index)(struct glbMaze *self,
		      const char *obj_id);

    int (*sort)(struct glbMaze *self);

    int (*search)(struct glbMaze *self,
		  struct glbData *data);

};

/* constructor */
extern int glbMaze_new(struct glbMaze **self);

#endif /* GLB_MAZE_H */
