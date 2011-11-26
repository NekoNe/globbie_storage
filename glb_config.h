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
enum { glb_OK, glb_FAIL, FAIL, glb_NOMEM, glb_NO_RESULTS, glb_IO_FAIL } glb_err_codes;
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


/* debugging output levels */
#define DEBUG_LEVEL_1 1
#define DEBUG_LEVEL_2 1
#define DEBUG_LEVEL_3 0
#define DEBUG_LEVEL_4 0
#define DEBUG_LEVEL_5 0


#define GLB_ID_MATRIX_DEPTH 3
#define MAX_MATRIX_SIZE 255 * 255 * 255 * 8 


#define GLB_TREE_OFFSET_SIZE 2

#define SPACE_CHAR 32

#define TMP_ID_BUF_SIZE 256

#define GLB_LOCSET_MAX_ENUM_VALUE 64

#define GLB_SET_POOL_SIZE 1024 * 10

#define GLB_LEAF_SIZE 100
/* alphanumeric symbols:
   0-9, A-Z, a-z */
#define GLB_RADIX_BASE 62

#define UCHAR_NUMVAL_RANGE (unsigned char)-1 + 1

#endif
