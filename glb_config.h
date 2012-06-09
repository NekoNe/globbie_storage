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
 *   -----------
 *   glb_config.h
 *   Globbie configuration settings
 */

#ifndef GLB_CONFIG_H
#define GLB_CONFIG_H

/* return error codes */
enum { glb_OK, glb_FAIL, glb_NOMEM, glb_NO_RESULTS, 
       glb_IO_FAIL, glb_EOB, glb_STOP, glb_NEED_WAIT } 
  glb_err_codes;

/* element status in the tree of changes */
enum { glb_PRESENCE, glb_DELETED, glb_IRRELEVANT, glb_IDXT_NTD, glb_IDXT_NEED_ROTATE } glb_tree_status_codes;
/* comparison codes */
enum {glb_EQUALS, glb_LESS, glb_MORE, glb_NOT_COMPARABLE } glb_comparison_codes; 

#ifdef WIN32
  #ifndef NO_BUILD_DLL
      #define EXPORT __declspec(dllexport)
  #else
      #define EXPORT __declspec(dllimport)
  #endif
#else
  #define EXPORT
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L

#include <stdbool.h>

#else

#undef bool
#undef true
#undef false

#define bool unsigned char
#define true  1
#define false 0

#endif

#define LOGIC_NOT 0
#define LOGIC_AND 1 
#define LOGIC_OR  2


#define GENERIC_TOPIC_NAME "Общее"

/* debugging output levels */
#define GLB_DEBUG_LEVEL_1 1
#define GLB_DEBUG_LEVEL_2 1
#define GLB_DEBUG_LEVEL_3 0
#define GLB_DEBUG_LEVEL_4 0
#define GLB_DEBUG_LEVEL_5 0

#define GLB_RESULT_BATCH_SIZE 20

#define GLB_RHANDLER_DEBUG_LEVEL_1 0
#define GLB_RHANDLER_DEBUG_LEVEL_2 0
#define GLB_RHANDLER_DEBUG_LEVEL_3 0
#define GLB_RHANDLER_DEBUG_LEVEL_4 0

#define GLB_COLLECTION_DEBUG_LEVEL_1 1
#define GLB_COLLECTION_DEBUG_LEVEL_2 1

#define GLB_ID_MATRIX_DEPTH 3


#define GLB_NAME_LENGTH 50

/* default numeric base */
#define GLB_NUM_ENCODE_BASE 10

#define GLB_CONC_PREFIX_NUM_DIGITS 3
#define GLB_CONC_PREFIX_DIGIT_SIZE 2
#define GLB_CONC_PREFIX_SIZE  GLB_CONC_PREFIX_NUM_DIGITS *  GLB_CONC_PREFIX_DIGIT_SIZE

#define GLB_TREE_OFFSET_SIZE 2

#define GLB_INDEX_ID_BATCH_SIZE 100

#define SPACE_CHAR 32

#define GLB_LOCSET_MAX_ENUM_VALUE 64

#define GLB_SET_POOL_SIZE 1024 * 10

#define GLB_LEAF_SIZE 10

/* number of leafs to read from index at once */
#define GLB_LEAF_CHUNK_SIZE 100

#define GLB_SIZE_OF_OFFSET sizeof(size_t)

#define GLB_ID_BLOCK_SIZE (GLB_ID_MATRIX_DEPTH * sizeof(char))

/* alphanumeric symbols:
   0-9, A-Z, a-z */
#define GLB_RADIX_BASE 62
#define GLB_ID_MAX_COUNT GLB_RADIX_BASE * GLB_RADIX_BASE * GLB_RADIX_BASE

#define UCHAR_NUMVAL_RANGE (unsigned char)-1 + 1

/* hash table sizes */
#define GLB_HUGE_DICT_SIZE 100000
#define GLB_LARGE_DICT_SIZE 10000
#define GLB_MEDIUM_DICT_SIZE 1000
#define GLB_SMALL_DICT_SIZE 100
#define GLB_TINY_DICT_SIZE 10

#define GLB_MAZE_ITEM_STORAGE_SIZE 1024 * 1024
#define GLB_MAZE_SPEC_STORAGE_SIZE 1024 * 1024
#define GLB_MAZE_LOC_STORAGE_SIZE 1024 * 1024


#define GLB_MAZE_NUM_AGENTS 10
#define GLB_MAZE_NUM_CACHE_SETS 1000

#define GLB_MAZE_MAX_DEPTH 3

#define GLB_TEMP_BUF_SIZE 1024
#define GLB_TEMP_SMALL_BUF_SIZE 64

#define GLB_RESULT_BUF_SIZE 1024 * 10 * sizeof(char)

#define GLB_LOC_REC_SIZE (sizeof(long) + sizeof(long))

#define GLB_MAX_CONC_UNITS 10000

#define GLB_LOCSET_BUF_SIZE GLB_MAX_CONC_UNITS * GLB_LOC_REC_SIZE

#define GLB_MAX_TEXT_BUF_SIZE 1024 * 1024

#define GLB_MAX_CONTEXTS 3

#define GLB_MAX_RESULT_SIZE 1000
#define GLB_CONTEXT_RADIUS 100

#define PARAGRAPH_BEGIN "<span>"
#define PARAGRAPH_END "</span>"

#define HIGHLIGHT_BEGIN "<b>"
#define HIGHLIGHT_END "</b>"

#define SPACE ".."


#endif
