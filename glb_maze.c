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

#include <fcntl.h>
#include <sys/stat.h>

#include <libxml/parser.h>

#include <db.h>

#include "glb_config.h"
#include "glb_maze.h"
#include "glb_set.h"
#include "glb_utils.h"
#include "glb_partition.h"
#include "glb_request_handler.h"
#include "oodict.h"

#define DEBUG_MAZE_LEVEL_1 1
#define DEBUG_MAZE_LEVEL_2 1
#define DEBUG_MAZE_LEVEL_3 1
#define DEBUG_MAZE_LEVEL_4 1
#define DEBUG_MAZE_LEVEL_5 0

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


/* сравнивает два отрезка. задаёт отношение порядка. */
static int 
glb_range_compare(const void *a, 
		  const void *b)
{
    size_t begin_a = ((size_t *)a)[0];
    size_t begin_b = ((size_t *)b)[0];
    
    return begin_a > begin_b ? 1 : -1;

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

    if (self->num_agents == self->max_agents) {
	/* TODO alloc only the sets with lower num objs */
	fprintf(stderr, "Maze's agent set storage capacity exceeded... Resize?\n");
	return NULL;
    }

    set = self->agent_set_storage[self->num_agents];
    self->num_agents++;

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

/* allocate setref */
static struct glbSetRef*
glbMaze_alloc_setref(struct glbMaze *self)
{
    struct glbSetRef *setref;

    if (self->num_setrefs == self->max_setrefs) {
	fprintf(stderr, "Maze's setref storage capacity exceeded... Resize?\n");
	return NULL;
    }

    setref = &self->setref_storage[self->num_setrefs];

    /* init null values */
    memset(setref, 0, sizeof(struct glbSetRef));

    self->num_setrefs++;

    return setref;
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

static struct glbSetRef*
glbMaze_find_setref(struct glbSetRef *ref,
		    const char *name)
{
    while (ref) {
	if (!strcmp(ref->name, name))
	    return ref;
	ref = ref->next;
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

    /*printf("  add complex \"%s\" as spec \"%s\"..\n", 
      name, topic_spec->name);*/

    item = NULL;

    if (topic_spec->items)
	item = glbMaze_find_item(topic_spec->items, name);


    /*if (!item) {
	item = glbMaze_alloc_item(self);
	if (!item) return glb_NOMEM;
	item->name = name;
	item->next = topic_spec->items;
	topic_spec->items = item;
    }
    else {
	printf("  spec already exists: %p!\n", item);
	}*/


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



/*  doc -- буффер с текстом. можно дать кусок документа
 *  с запасом справа и слева, желательно. и желательно  
 *  чтобы этот запас совпадал с GLB_CONTEXT_RADIUS
 *
 *  ranges -- расположение подсвечиваемых областей. обязательно 
 *  должно быть упорядоченно по возрастанию. порядок задан функцией
 *  glb_range_compare
 *
 *  ranges -- кол-во отрезков * кол-во полей в записи
 */

static int 
glbMaze_build_context(struct glbMaze *self,	
		      struct glbRequestHandler *request,
		      char *result)
{
    char buf[GLB_TEMP_BUF_SIZE * 10];
    size_t buf_size = 0;
    char *s;

    char *doc = self->text;
    size_t *ranges = self->result_ranges;
    size_t ranges_count = request->set_pool_size * 2; 

    /*char *result = self->curr_results_buf;*/

    size_t results_size = 0;

    size_t max_size; /* result always ends with PARAGRAPTH_END */

    size_t space_size = strlen(SPACE);

    /* сколько сейчас уже записано */
    size_t actual_size = 0;
    
    /* сколько прибавится к буферу, если завершить текущую операцию */
    size_t predicted_size = space_size + GLB_CONTEXT_RADIUS;
    
    /* сколько до следующего концепта */
    size_t next_offset = 0; 

    /* запоминаем, где закончили читать контекст на прошлом шаге*/
    size_t last_offset = 0;

    size_t begin_pos = 0;
    size_t end_pos = 0;

    size_t i;
   
    /* резервируем место под закрывающий тег */
    max_size = GLB_MAX_RESULT_SIZE - strlen(PARAGRAPH_END);

    /* сразу записываем открывающий тег */
    strcat(result, PARAGRAPH_BEGIN);
    actual_size += strlen(PARAGRAPH_BEGIN);
    
    for (i = 0; i < ranges_count; i += 2) {
	begin_pos = ranges[i];
	end_pos = ranges[i + 1];

        /* левый контекст */
        /* контексты не пересеклись */
        if (begin_pos > (last_offset + GLB_CONTEXT_RADIUS)) {
	    /* исчерпан ресурс для записи? */
            if (actual_size + predicted_size >= max_size) break;
            
            /* поставим разделитель и распечатаем левый контекст */
            /* разделитель */
            strcat(result, SPACE);

	    /* от крайнего слева пробела */
	    memcpy(buf, doc + (begin_pos - GLB_CONTEXT_RADIUS), GLB_CONTEXT_RADIUS);
	    buf[GLB_CONTEXT_RADIUS] = '\0';
	    s = strstr(buf, " ");
	    if (!s) s = buf;

            strcat(result, s);

            actual_size += strlen(s) + space_size;
        } else {
            /* контексты пересеклись */
            predicted_size = begin_pos - last_offset;
            if (actual_size + predicted_size >= max_size) break;
            
            strncat(result, doc + last_offset, begin_pos - last_offset);

            actual_size += predicted_size;
        }

        /* концепт */

        predicted_size = strlen(HIGHLIGHT_BEGIN) +\
	    strlen(HIGHLIGHT_END) + end_pos - begin_pos + 1;
        if (actual_size + predicted_size >= max_size) break;

        strcat(result, HIGHLIGHT_BEGIN);

        /* сам концепт */
        strncat(result, doc + begin_pos, end_pos - begin_pos + 1);

        strcat(result, HIGHLIGHT_END);
        actual_size += predicted_size;
       
        /* правый контекст */

        /* если залезли на следующий концепт, то пишем ровно до него */
        
        next_offset = self->text_size;

        /* есть ли следующий концепт */
        if (i + 2 < ranges_count) 
            next_offset = ranges[i + 2];
        
        /* если заползаем на следующий концепт */
        if (end_pos + GLB_CONTEXT_RADIUS >= next_offset) {
            
            predicted_size = next_offset - end_pos - 1;
            if (actual_size + predicted_size >= max_size) break;
            
            strncat(result, doc + end_pos + 1, predicted_size);

            actual_size += predicted_size; 
            last_offset = end_pos + next_offset - end_pos ;
        } else {
            
            predicted_size = GLB_CONTEXT_RADIUS + space_size;
            if (actual_size + predicted_size >= max_size) break;

	    memcpy(buf, doc + (end_pos + 1), GLB_CONTEXT_RADIUS);
	    buf[GLB_CONTEXT_RADIUS] = '\0';

	    s = strrchr(buf, ' ');
	    if (s) *s = '\0';
	    buf_size = strlen(buf);

            strcat(result, buf);
                        
            /* разделитель */
            strcat(result, SPACE);
	    
            actual_size += buf_size + space_size;
            
            last_offset = end_pos + 1 + buf_size;
        }
    }

    glb_remove_nonprintables(result);

close_tag:

    /* добавим закрывающий тег */
    strcat(result, PARAGRAPH_END);

    return glb_OK;
}


/*  ищу оптималный контекст */

/*  ranges -- список оффсетов [номер концепта, номер оффсета]
 *
 *  offset_num -- сколько у определенного концепта оффсетов * 2
 *
 *  concept num -- сколько концептов
 *
 *  i -- с какого оффсета начинаем поиск. относится к нулевому концепту
 *  желательно самому редкому.
 *
 */

static int 
glbMaze_find_context(struct glbMaze *self,	
		     struct glbRequestHandler *request,  
		     size_t start_from)
{
    size_t **ranges = self->locsets;
    size_t *offset_num = self->num_locs; 
    size_t num_concepts =  request->set_pool_size;
    size_t *result_ranges = self->result_ranges;

    size_t j; /* номер списка офсетов */
    size_t k; /* бежим по списку офсетов*/

    size_t begin; /* начало отрезка*/
    size_t end; /* конец отрезка */
    
    size_t area; /* "площадь", которую покрывают два отрезка */
    size_t new_area; 

    const size_t num_fields = GLB_LOC_REC_NUM_FIELDS;

    /* отрезок из первого концепта */
    begin = ranges[0][start_from * num_fields];
    
    end = ranges[0][start_from * num_fields + 1];
    area = end - begin + 1;

    result_ranges[0] = begin;
    result_ranges[1] = end;

    /* последовательно находим ближайшие к i-ому отрезку*/
    /* сначала для первого j  находим ближний, потом для i и j-первого находим ближний...*/
    for (j = 1; j < num_concepts; j++) {

        /* рассчитаем, сколько занимает исходный отрезок вместе с тестируемым */
        area = GLB_CONTEXT_AREA(begin, end, ranges[j][0], ranges[j][1]);        

        result_ranges[num_fields * j] = ranges[j][0];
        result_ranges[num_fields * j + 1] = ranges[j][1];
        
        for (k = num_fields; k < offset_num[j]; k += num_fields) {
            
            /* так получаем отрезок, 
              с которым исходный занимает наименьшую площадь */
            new_area =  GLB_CONTEXT_AREA(begin, end, ranges[j][k], ranges[j][k + 1]);        
           
            if (new_area < area) {
                result_ranges[num_fields * j] = ranges[j][k];
                result_ranges[num_fields * j + 1] = ranges[j][k + 1];
                area = new_area;
            }
        }
        /* исходный отрезок совмещаем с отрезком,
         * полученным чуть выше, и повторяем дальше для него */
        begin = GLB_MIN(begin, result_ranges[num_fields * j]);
        end = GLB_MAX(end, result_ranges[num_fields * j + 1]);
    }

    /* сортировка */
    qsort(result_ranges, num_concepts, num_fields * sizeof(size_t), glb_range_compare);
    
    return 0;
}



static int
glbMaze_read_refs(struct glbMaze *self,
		  const char *rec,
		  size_t rec_size,
		  struct glbMazeItem *item)
{
    struct glbSetRef *ref = NULL;
    char *ref_name = NULL;
    
    int ret = glb_OK;

    long num_items = 0;

    /* read subclass refs */


    /*if (item->refs)
	ref = glbMaze_find_setref(item->refs, ref_name);

	    if (!ref) {
		ref = glbMaze_alloc_setref(self);
		if (!ref) goto error;

		ref->name = strdup(ref_name);

		ref->next = item->refs;
		item->refs = ref;
	    }

	    ret = glb_get_xmlattr_num(cur_node, "num", &num_items);
	    if (ret != glb_OK) continue;

	    ref->num_items += num_items;

	    if (DEBUG_MAZE_LEVEL_4)
		printf("   == REF: %s [%lu]\n", 
		       ref_name, (unsigned long)ref->num_items);

	}

    }
    */

    ret = glb_OK;

error:
    if (ref_name)
	xmlFree(ref_name);


    return ret;
}



static int 
glbMaze_add_conc(struct glbMaze *self,
		 const char *name,
		 size_t name_size,
		 const char *rec,
		 size_t rec_size,
		 const char *obj_id)
{
    struct glbMazeItem *item;
    struct glbMazeSpec *spec;
    struct glbMazeLoc *loc;
    int ret;

    /* registration of a new root item */
    item = self->item_dict->get(self->item_dict, name);
    if (!item) {
	item = glbMaze_alloc_item(self);
	if (!item) return glb_NOMEM;

	item->name = malloc(name_size + 1);
	if (!item->name) return glb_NOMEM;
	item->name[name_size] = '\0';
	memcpy(item->name, name, name_size);
	item->name_size = name_size;

	ret = self->item_dict->set(self->item_dict,
				   name, item);

	self->item_index[self->num_root_items] = item;
	self->num_root_items++;

	if (DEBUG_MAZE_LEVEL_4)
	    printf("  ++ root registration of item %s  REC: %s\n",
		   item->name, rec);
    }

    

    /* read subclass refs */
    glbMaze_read_refs(self, rec, rec_size, item);


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
glbMaze_read_text(struct glbMaze *self,	
		  const char *obj_id)
{
    char path[GLB_TEMP_BUF_SIZE];
    size_t res;
    int fd = 0;
    int i, ret;

    path[0] = '\0';

    /* build path to text  */
    glb_make_id_path(path,  self->storage_path, obj_id, "text");

    if (DEBUG_MAZE_LEVEL_5)
	printf("\n  ...reading text from: \"%s\"\n", path);

    fd = open((const char*)path, O_RDONLY);
    if (fd < 0) {
	close(fd);
	return glb_IO_FAIL;
    }

    res = read(fd, self->text, GLB_MAX_TEXT_BUF_SIZE);
    if (!res) {
	close(fd);
	return glb_IO_FAIL;
    }

    self->text[res] = '\0';
    self->text_size = res;

    close(fd);

    return glb_OK;
}


static int 
glbMaze_build_contexts(struct glbMaze *self,	
		       struct glbRequestHandler *request,
		       const char *obj_id)
{
    size_t max_locs;
    int i, j, ret;

    char buf[GLB_TEMP_BUF_SIZE * 10];
    
    long curr_loc = -1;
    size_t begin_loc = 0;
    const char *output;
    size_t output_size;
    const size_t num_fields = GLB_LOC_REC_NUM_FIELDS;

    int context_count = 0;

    max_locs = self->num_locs[0];

    /* reading original text into memory */
    ret = glbMaze_read_text(self, obj_id);
    if (ret != glb_OK) return ret;

    if (DEBUG_MAZE_LEVEL_2)
	printf("\n   .. building contexts.. (max: %d)\n", max_locs);

    buf[0] = '\0';

    for (i = 0; i < max_locs; i++) {
	if (i == GLB_MAX_CONTEXTS) break;

	ret = glbMaze_find_context(self, request, i);
	if (ret != glb_OK) return glb_FAIL;

	if (DEBUG_MAZE_LEVEL_2)
	    printf("    == linear positions in text:\n");

	for (j = 0; j < request->set_pool_size; j++ ) {
	    begin_loc = self->result_ranges[num_fields * j];

	    printf(" (%d, %d) ",
		   self->result_ranges[num_fields * j],
		   self->result_ranges[num_fields * j + 1]);
	    if (curr_loc == begin_loc) {
		if (DEBUG_MAZE_LEVEL_2)
		    printf("    ?? Doublet found: %lu? :((\n", begin_loc);
		return glb_FAIL;
	    }
	    curr_loc = self->result_ranges[num_fields * j];

	}

	if (DEBUG_MAZE_LEVEL_2)
	    printf("\n"); 

	ret = glbMaze_build_context(self, request, buf); 
	if (ret != glb_OK) continue;

	context_count++;
    }

    if (!context_count) return glb_FAIL;

    output = "<div class=\\\"msg_row\\\">"
	"<div class=\\\"msg_row_title\\\">%s</div>"
	"<div class=\\\"msg_row_desc\\\">";

    sprintf(self->curr_results_buf, output, obj_id);
    output_size = strlen(self->curr_results_buf);
    self->curr_results_buf += output_size;
    self->results_size += output_size;


    sprintf(self->curr_results_buf, buf);
    output_size = strlen(buf);
    self->curr_results_buf += output_size;
    self->results_size += output_size;

    output = "</div></div>";
    output_size = strlen(output);
    sprintf(self->curr_results_buf, output);
    self->curr_results_buf += output_size;
    self->results_size += output_size;
 

    printf("\n\n   OUTPUT RESULTS:\n %s\n\n", 
	   self->results);
 
    return glb_OK;

}

static int 
glbMaze_fetch_obj(struct glbMaze *self,	
		  struct glbRequestHandler *request,
		  const char *obj_id,
		  struct glbData *data)
{
    char path[GLB_TEMP_BUF_SIZE];
    char prefix[GLB_TEMP_SMALL_BUF_SIZE];
    char filename[GLB_TEMP_BUF_SIZE];
    char rec[GLB_LOC_REC_SIZE];

    struct glbSet *set;
    size_t *locset;
    size_t *num_locs;

    int fd = 0;
    size_t res;
    int i, j, ret;

    /* reading locsets */
    for (i = 0; i < request->set_pool_size; i++) {
	set = request->set_pool[i];
	locset = self->locsets[i];
	num_locs = &self->num_locs[i];
	*num_locs = 0;

	/* extracting prefix from concept name */
	glb_get_conc_prefix(set->name, set->name_size, prefix);

	/* build the filename */
	sprintf(filename, "locs/%s/%s.set", prefix, set->name);
	glb_make_id_path(path,
			 self->storage_path,
			 obj_id, 
			 (const char*)filename);

	if (DEBUG_MAZE_LEVEL_2)
	    printf("    .. reading locs from file \"%s\"...\n", path);

	fd = open(path, O_RDONLY);
	if (fd < 0) {
	    printf("    -- couldn\'t open file %s :((\n", path);
	    close(fd);
	    return glb_IO_FAIL;
	}

	res = read(fd, locset, GLB_LOCSET_BUF_SIZE);
	if (!res) {
	    close(fd);
	    return glb_FAIL;
	}

	*num_locs = res / GLB_LOC_REC_SIZE;
	close(fd);

    }

    if (DEBUG_MAZE_LEVEL_5)
	printf("    ++ locsets read success!\n");

    ret = glbMaze_build_contexts(self, request, obj_id); 
    if (ret != glb_OK) return ret;

    return glb_OK;
}


static int
glbMaze_fetch_results(struct glbMaze *self, 
			struct glbRequestHandler *request,
			struct glbData *data)
{
    char buf[GLB_ID_MATRIX_DEPTH + 1];
    char *ids;
    size_t num_ids;
    int i, ret;

    buf[GLB_ID_MATRIX_DEPTH] = '\0';

    ids = request->result->ids;
    num_ids = request->result->answer_actual_size;

    /* print all ids */
    if (DEBUG_MAZE_LEVEL_2) {
	for (i = 0; i < num_ids; i++) {
	    memcpy(buf,
		   ids + i * GLB_ID_MATRIX_DEPTH, 
		   GLB_ID_MATRIX_DEPTH);
	    printf(" [%s] ", buf);
	}
	puts("");
    }

    for (i = 0; i < num_ids; i++) {
	memcpy(buf,
	       ids + i * GLB_ID_MATRIX_DEPTH, 
	       GLB_ID_MATRIX_DEPTH);

	/* для каждого id читаем его локальный индекс и текст */
	ret = glbMaze_fetch_obj(self, request, buf, data);

	printf("   .. fetch obj_id [%s]: %d\n", buf, ret);

	if (ret != glb_OK) continue;

	data->num_results++;
    }

    return glb_OK;
}


static int 
glbMaze_add_domain_terms(struct glbMaze *self,
			 struct glbSet *set,
			 const char *domain_name)
{
    struct glbMazeItem *item;
    struct glbSetRef *ref;
    int ret;

    if (DEBUG_MAZE_LEVEL_2)
	printf("\n    .. add terms from domain \"%s\"..\n", 
	       domain_name);

    item = self->item_dict->get(self->item_dict, domain_name);
    if (!item) {
	if (DEBUG_MAZE_LEVEL_2)
	    printf("\n  .. no objs found in domain \"%s\"\n", domain_name);

	return glb_NO_RESULTS;
    }


    if (item->refs) {
	
	printf("    ++ domain %s has refs!\n", domain_name);

	for (ref = item->refs; ref; ref = ref->next) {
	    printf("   == REF: %s [%lu]\n", 
		   ref->name, (unsigned long)ref->num_items);


	}

	set->refs = item->refs;
    }

    return glb_OK;
}


static int 
glbMaze_add_search_term(struct glbMaze *self, 
			const char *conc_name,
			const char *domain_name)
{
    struct glbMazeItem *item;
    struct glbSet *set;
    int ret;

    if (DEBUG_MAZE_LEVEL_2)
	printf("\n    .. add search term %s [domain: %s]\n", 
	       conc_name, domain_name);

    item = self->item_dict->get(self->item_dict, conc_name);
    if (!item) {
	if (DEBUG_MAZE_LEVEL_2)
	    printf("\n  .. no such set found: %s\n", conc_name);

	return glb_NO_RESULTS;
    }

    /* take one of the agents */
    set = glbMaze_alloc_agent_set(self);
    if (!set) return glb_NOMEM;

    ret = set->init(set,
		    (const char*)self->path, 
		    item->name, item->name_size);
    if (ret != glb_OK) return ret;

    set->num_objs = item->num_objs;

    item->num_requests++;


    if (domain_name)
	glbMaze_add_domain_terms(self, set, domain_name);


    self->search_set_pool[self->search_set_pool_size] = set;
    self->search_set_pool_size++;

    return glb_OK;
}


static int
glbMaze_search(struct glbMaze *self, 
	       struct glbData *data)
{
    char buf[GLB_ID_MATRIX_DEPTH + 1];
    xmlDocPtr doc;
    xmlNodePtr root, cur_node;
    struct glbSet *set;
    struct glbRequestHandler *request = NULL;

    char *conc_name = NULL;
    char *domain_name = NULL;

    int ret = glb_OK;
    int i;

    if (DEBUG_MAZE_LEVEL_1)
	printf("    .. Maze %d:  search in progress...\n", 
	       self->id);

    doc = xmlReadMemory(data->interp, 
			data->interp_size,
			"none.xml", NULL, 0);
    if (!doc) {
        fprintf(stderr, "    -- Failed to parse interp.. :(\n");
	return glb_FAIL;
    }

    if (DEBUG_MAZE_LEVEL_3)
	printf("    ++ Maze %d: interp XML parse OK!\n", 
	       self->id);

    root = xmlDocGetRootElement(doc);
    if (!root) {
	fprintf(stderr,"    -- empty XML document :(\n");
	ret = glb_FAIL;
	goto error;
    }

    if (xmlStrcmp(root->name, (const xmlChar *) "concepts")) {
	fprintf(stderr,"    -- Document of the wrong type: the root node " 
		" must be \"concepts\"\n");
	ret = glb_FAIL;
	goto error;
    }

    /* reset agents */
    self->num_agents = 0;
    self->search_set_pool_size = 0;

    for (cur_node = root->children; cur_node; cur_node = cur_node->next) {
        if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name,
			(const xmlChar *)"conc"))) {

	    conc_name = (char *)xmlGetProp(cur_node,
					   (const xmlChar *)"n");
	    if (!conc_name) goto error;

	    domain_name = (char *)xmlGetProp(cur_node,
				       (const xmlChar *)"domain");

	    ret = glbMaze_add_search_term(self,
					  (const char*)conc_name,
					  (const char*)domain_name);
	    if (ret != glb_OK) goto error;

	    xmlFree(conc_name);
	    conc_name = NULL;

	    if (domain_name) {
		xmlFree(domain_name);
		domain_name = NULL;
	    }

	}
    }

    printf("\n    ++ NUM INTERSECTION SETS: %d\n", 
	   self->search_set_pool_size);

    if (self->search_set_pool_size == 0) return glb_NO_RESULTS;

    /* sorting the sets by size */
    qsort(self->search_set_pool,
	  self->search_set_pool_size, 
	  sizeof(struct glbSet*), glbSet_cmp);

    if (DEBUG_MAZE_LEVEL_2) {
	printf("    == Sets after sorting:\n");
	for (i = 0; i < self->search_set_pool_size; i++) {
	    set = self->search_set_pool[i];
	    printf("      %d: %s [num objs: %d]\n", 
		   i, set->name, set->num_objs);
	}
    }

    ret = glbRequestHandler_new(&request,
				GLB_RESULT_BATCH_SIZE,
				0,
				self->search_set_pool,
				self->search_set_pool_size);
    if (ret != glb_OK) return ret;

    /* main job: logical AND intersection */
    if (DEBUG_MAZE_LEVEL_2)
	puts("\n    ..  LOGICAL INTERSECTION  in progress..");

    ret = request->intersect(request);

    if (!request->result->answer_actual_size) {
	ret = glb_NO_RESULTS;
	goto error;
    }

    if (DEBUG_MAZE_LEVEL_1) {
	printf("    ++ LOGICAL INTERSECTION RESULTS: %d\n",
	       request->result->answer_actual_size);
    }

    /* reset operational buffers */
    self->text[0] = '\0';
    self->results[0] = '\0';
    self->curr_results_buf = self->results;
    self->results_size = 0;

    ret = glbMaze_fetch_results(self, request, data);
    if (ret != glb_OK) goto error;

    if (DEBUG_MAZE_LEVEL_1) {
	printf("    ++ Search query yielded the following results: %d\n\n",
	       data->num_results);
    }

error:

    if (conc_name)
	xmlFree(conc_name);

    if (domain_name)
	xmlFree(domain_name);

    if (request)
	request->del(request);

    xmlFreeDoc(doc);

    return ret;
}

static int 
glbMaze_read_index(struct glbMaze *self,
		   const char *obj_id)
{
    DB *dbp = NULL;
    DBC *dbc;
    DBT db_key, db_data;

    char buf[GLB_TEMP_BUF_SIZE];

    size_t res;
    int fd = 0;
    int i, ret = glb_OK;

    buf[0] = '\0';
    glb_make_id_path(buf,
		     self->storage_path,
		     obj_id, 
		     "index.db");

    if (DEBUG_MAZE_LEVEL_2)
	printf("\n  ...reading index from \"%s\" ", buf);

    /* create and initialize database object */
    if ((ret = db_create(&dbp, NULL, 0)) != 0) {
	fprintf(stderr,
		"db_create: %s\n", db_strerror(ret));
	goto error;
    }

    dbp->set_errfile(dbp, stderr);
    dbp->set_errpfx(dbp, "Maze");

    if ((ret = dbp->set_pagesize(dbp, 1024)) != 0) {
	dbp->err(dbp, ret, "set_pagesize");
	goto error;
    }

    if ((ret = dbp->set_cachesize(dbp, 0, 1024 * 1024, 0)) != 0) {
	dbp->err(dbp, ret, "set_cachesize");
	goto error;
    }

    /* open the database */
    ret = dbp->open(dbp,        /* DB structure pointer */ 
		    NULL,       /* transaction pointer */ 
		    (const char*)buf, /* filename */
		    NULL,       /* optional logical database name */
		    DB_HASH,    /* database access method */
		    DB_RDONLY,  /* open flags */
		    0664);      /* file mode (using defaults) */

    if (ret != oo_OK) {
	dbp->err(dbp, ret, "%s: open", (const char*)buf);
	(void)dbp->close(dbp, 0);
	return oo_FAIL;
    }

    /* initialize the DBTs */
    memset(&db_key, 0, sizeof(DBT));
    memset(&db_data, 0, sizeof(DBT));

    dbp->cursor(dbp, NULL, &dbc, 0);

    while ((ret = dbc->c_get(dbc, 
			     &db_key, &db_data, 
			     DB_NEXT)) == glb_OK) {

	ret = glbMaze_add_conc(self,
			       (const char*)db_key.data,
			       (size_t)db_key.size,
			       (const char*)db_data.data,
			       (size_t)db_data.size,
			       obj_id);

	/*memcpy(buf, db_key.data, db_key.size);
	buf[db_key.size] = '\0';
        printf("  ITEM: \"%s\"\n", buf);
	memcpy(buf, db_data.data, db_data.size);
	buf[db_data.size] = '\0';
        printf("  VALUE: \"%s\"\n\n", buf);
	*/
    }

    dbc->c_close(dbc);
    ret = glb_OK;

 error:

    if (dbp)
	dbp->close(dbp, 0);

    return ret;
}



static int 
glbMaze_init(struct glbMaze *self)
{
    char id_buf[GLB_ID_MATRIX_DEPTH + 1];
    int ret;

    memset(id_buf, '0', GLB_ID_MATRIX_DEPTH);
    id_buf[GLB_ID_MATRIX_DEPTH] = '\0';

    /* load existing indices */

    if (DEBUG_MAZE_LEVEL_2)
	printf("\n  .. MAZE is loading indices from %s to %s...\n",
	       id_buf, self->partition->curr_obj_id);

    while (1) {
	inc_id(id_buf);
	ret = compare(id_buf, self->partition->curr_obj_id);
	if (ret == glb_MORE) break;

	ret = glbMaze_read_index(self, (const char*)id_buf);

	if (DEBUG_MAZE_LEVEL_2) {
	    if (ret == glb_OK) 
		puts(" [OK]");
	    else
		puts(" [FAIL]");
	}
    }


    return glb_OK;
}

/* constructor */
extern int
glbMaze_new(struct glbMaze **maze)
{
    int i, ret;
    struct glbSet *set;
    size_t *locset;

    struct glbMaze *self = malloc(sizeof(struct glbMaze));
    if (!self) return glb_NOMEM;

    memset(self, 0, sizeof(struct glbMaze));

    /* allocate items */
    self->item_storage = malloc(sizeof(struct glbMazeItem) *\
				GLB_MAZE_ITEM_STORAGE_SIZE);
    if (!self->item_storage) {
	ret = glb_NOMEM;
	goto error;
    }

    self->item_index = malloc(sizeof(struct glbMazeItem*) *\
				GLB_MAZE_ITEM_STORAGE_SIZE);
    if (!self->item_index) {
	ret = glb_NOMEM;
	goto error;
    }

    self->item_storage_size = GLB_MAZE_ITEM_STORAGE_SIZE;

    ret = ooDict_new(&self->item_dict, GLB_LARGE_DICT_SIZE);
    if (ret != glb_OK) goto error;

    /* allocate specs */
    self->spec_storage = malloc(sizeof(struct glbMazeSpec) *\
				GLB_MAZE_SPEC_STORAGE_SIZE);
    if (!self->spec_storage) {
	ret = glb_NOMEM;
	goto error;
    }
    self->spec_storage_size = GLB_MAZE_SPEC_STORAGE_SIZE;




    /* text content buffer */
    self->text = malloc(GLB_MAX_TEXT_BUF_SIZE + 1);
    if (!self->text) {
	ret = glb_NOMEM;
	goto error;
    }
    
    /* results buffer */
    self->results = malloc(GLB_RESULT_BUF_SIZE + 1);
    if (!self->results) {
	ret = glb_NOMEM;
	goto error;
    }
    self->curr_results_buf = self->results;

    /* allocate agent locsets */
    self->locsets = malloc(GLB_MAZE_NUM_AGENTS *\
			   sizeof(size_t *));
    if (!self->locsets) {
	ret = glb_NOMEM;
	goto error;
    }

    self->num_locs = malloc(GLB_MAZE_NUM_AGENTS *\
			   sizeof(size_t));
    if (!self->num_locs) {
	ret = glb_NOMEM;
	goto error;
    }

    self->result_ranges =  malloc(GLB_MAZE_NUM_AGENTS *\
				  GLB_LOC_REC_SIZE);
    if (!self->result_ranges) {
	ret = glb_NOMEM;
	goto error;
    }

    for (i = 0; i < GLB_MAZE_NUM_AGENTS; i++) {
	locset = malloc(GLB_LOCSET_BUF_SIZE);
	if (!locset) {
	    ret = glb_NOMEM;
	    goto error;
	}
	self->locsets[i] = locset;
    }

    /* allocate agent sets */
    self->agent_set_storage = malloc(GLB_MAZE_NUM_AGENTS *\
				     sizeof(struct glbSet*));
    if (!self->agent_set_storage) {
	ret =  glb_NOMEM;
	goto error;
    }

    for (i = 0; i < GLB_MAZE_NUM_AGENTS; i++) {
	ret = glbSet_new(&set);
	if (ret != glb_OK) goto error;
	self->agent_set_storage[i] = set;
    }
    self->max_agents = GLB_MAZE_NUM_AGENTS;


    /* allocate setrefs */
    self->setref_storage = malloc(sizeof(struct glbSetRef) *\
				GLB_MAZE_SETREF_STORAGE_SIZE);
    if (!self->setref_storage) {
	ret = glb_NOMEM;
	goto error;
    }
    self->max_setrefs = GLB_MAZE_SETREF_STORAGE_SIZE;



    /* set cache */
    self->search_set_pool = malloc(GLB_MAZE_NUM_CACHE_SETS *\
				     sizeof(struct glbSet*));
    if (!self->search_set_pool) {
	ret = glb_NOMEM;
	goto error;
    }
    self->max_search_set_pool_size = GLB_MAZE_NUM_CACHE_SETS;

    /* allocate cache sets */
    for (i = 0; i < GLB_MAZE_NUM_CACHE_SETS; i++) {
	ret = glbSet_new(&set);
	if (ret != glb_OK) goto error;

	/* first set */
	if (!self->head) {
	    self->head = set;
	    self->tail = set;
	    continue;
	}
	
	set->next = self->head;
	self->head = set;
    }

    self->cache_set_storage_size = GLB_MAZE_NUM_CACHE_SETS;

    self->del           = glbMaze_del;
    self->init          = glbMaze_init;
    self->str           = glbMaze_str;

    self->read_index    = glbMaze_read_index;
    self->sort          = glbMaze_sort_items;
    self->search        = glbMaze_search;


    *maze = self;

    return glb_OK;


error:
    glbMaze_del(self);
    
    return ret;
}
