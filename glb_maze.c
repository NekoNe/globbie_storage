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

#include "glb_config.h"
#include "glb_maze.h"
#include "glb_set.h"
#include "glb_utils.h"
#include "glb_request_handler.h"
#include "oodict.h"

#define DEBUG_MAZE_LEVEL_1 0
#define DEBUG_MAZE_LEVEL_2 0
#define DEBUG_MAZE_LEVEL_3 0
#define DEBUG_MAZE_LEVEL_4 0
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

    /*printf("  add complex \"%s\" as spec \"%s\"..\n", 
      name, topic_spec->name);*/

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
    
    /* сколько прибавится к буфферу, если завершить текущую операцию */
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

    size_t j; /* номер списка оффсетов */
    size_t k; /* бежим по списку оффсетов*/
    
    size_t begin; /* начало отрезка*/
    size_t end; /* конец отрезка */
    
    size_t area; /* "площадь", которую покрывают два отрезка */
    size_t new_area; 

    /* отрезок из первого концепта */
    begin = ranges[0][start_from * 2];
    
    end = ranges[0][start_from * 2 + 1];
    area = end - begin + 1;

    result_ranges[0] = begin;
    result_ranges[1] = end;

    /* последовательно находим ближайшие к i-ому отрезку*/
    /* сначала для первого j  находим ближний, потом для i и j-первого находим ближний...*/
    for (j = 1; j < num_concepts; j++) {

        /* рассчитаем, сколько занимает исходный отрезок вместе с тестируемым */
        area = GLB_CONTEXT_AREA(begin, end, ranges[j][0], ranges[j][1]);        

        result_ranges[2 * j] = ranges[j][0];
        result_ranges[2 * j + 1] = ranges[j][1];
        
        for (k = 2; k < offset_num[j]; k += 2) {
            
            /* так получаем отрезок, 
              с которым исходный занимает наименьшую площадь */
            new_area =  GLB_CONTEXT_AREA(begin, end, ranges[j][k], ranges[j][k + 1]);        
           
            if (new_area < area) {
                result_ranges[2 * j] = ranges[j][k];
                result_ranges[2 * j + 1] = ranges[j][k + 1];
                area = new_area;
            }
        }
        /* исходный отрезок совмещаем с отрезком,
         * полученным чуть выше, и повторяем дальше для него */
        begin = GLB_MIN(begin, result_ranges[2 * j]);
        end = GLB_MAX(end, result_ranges[2 * j + 1]);
    }

    /* сортировка */
    qsort(result_ranges, num_concepts, 2 * sizeof(size_t), glb_range_compare);
    
    return 0;
}



static int 
glbMaze_add_locs(struct glbMaze *self,
		 xmlNodePtr input_node,
		 const char *filename)
{
    xmlNodePtr cur_node;
    long b = 0;
    long e = 0;

    long curr_begin = 0;

    char rec[GLB_LOC_REC_SIZE];
    size_t res;
    int fd;

    int ret;

    if (DEBUG_MAZE_LEVEL_4)
	printf("  .. writing to file: %s\n", filename);

    fd = open(filename, O_WRONLY | O_CREAT, 0644);
    if (fd < 0) return glb_IO_FAIL;

    for (cur_node = input_node;
	 cur_node; 
	 cur_node = cur_node->next) {
	
	if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"loc"))) {
		    
	    ret = glb_get_xmlattr_num(cur_node, "b", &b);
	    if (ret != glb_OK) return ret;

	    /* skipping over doublets */
	    if (b && b == curr_begin) continue;

	    ret = glb_get_xmlattr_num(cur_node, "e", &e);
	    if (ret != glb_OK) return ret;

	    if (b < 0 || e <= 0 || e <= b) return glb_FAIL;

	    memcpy(rec, &b, sizeof(long));
	    memcpy(rec + sizeof(long), &e, sizeof(long));

	    /* writing loc */
	    write(fd, rec, GLB_LOC_REC_SIZE);
	    curr_begin = b;
	}
    }
    close(fd);

    return glb_OK;
}


static int 
glbMaze_save_local_index(struct glbMaze *self,
			xmlNodePtr input_node,
			const char *name,
			size_t name_size,
			const char *obj_id)
{
    char path[GLB_TEMP_BUF_SIZE];
    char filename[GLB_TEMP_BUF_SIZE];
    char prefix[GLB_TEMP_SMALL_BUF_SIZE];

    char buf[GLB_TEMP_SMALL_BUF_SIZE];


    xmlNodePtr cur_node;
    char *value;
    int ret;

    glb_get_conc_prefix(name, name_size, prefix);

    sprintf(buf, "locs/%s", prefix); 

    path[0] = '\0';
    ret = glb_make_id_path(path, 
			   self->storage_path, 
			   obj_id, 
			   (const char*)buf);
    if (ret != glb_OK) return ret;

    if (DEBUG_MAZE_LEVEL_5)
	printf("\n   ... making directory: %s\n", path);

    ret = glb_mkpath(path, 0777);
    if (ret != glb_OK) return ret;

    filename[0] = '\0';
    sprintf(filename, "%s/%s.set", path, name); 

    for (cur_node = input_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"locs"))) {
	    ret = glbMaze_add_locs(self, cur_node->children, filename);
	    if (ret != glb_OK) continue;
	}
    }

    return glb_OK;
}

static int 
glbMaze_add_conc(struct glbMaze *self,
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

	if (DEBUG_MAZE_LEVEL_4)
	    printf("  ++ root registration of item %s\n", 
		   item->name);
    }

    /* add obj_id to appropriate set */

    /* set already in cache? */
    if (item->set) {
	set = item->set;
    }
    else {
	self->num_agents = 0;

	/* take one of the agents */
	set = glbMaze_alloc_agent_set(self);
	if (!set) return glb_NOMEM;
	ret = set->init(set,
			(const char*)self->path, 
			(const char*)item->name,
			item->name_size);
	if (ret != glb_OK) return ret;
    }


    if (DEBUG_MAZE_LEVEL_5)
	printf("   .. Set is adding obj \"%s\" to %s/%s...\n", 
	       obj_id, self->path, item->name );

    ret = set->add(set, obj_id);
    if (ret != glb_OK) return ret;

    ret = glbMaze_save_local_index(self, 
				   input_node, 
				   (const char*)item->name,
				   item->name_size,
				   obj_id);
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
    
    size_t curr_loc = 0;
    size_t begin_loc = 0;
    const char *output;
    size_t output_size;

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
	    begin_loc = self->result_ranges[2 * j];

	    printf(" (%d, %d) ",
		   self->result_ranges[2 * j],
		   self->result_ranges[2 * j + 1]);
	    if (curr_loc == begin_loc) {
		if (DEBUG_MAZE_LEVEL_2)
		    printf("    ?? Doublet found: %lu? :((\n", begin_loc);
		return glb_FAIL;
	    }
	    curr_loc = self->result_ranges[2 * j];

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

	if (ret != glb_OK) return ret;

	data->num_results++;
    }

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
		    item->name, item->name_size);
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
    char buf[GLB_ID_MATRIX_DEPTH + 1];
    xmlDocPtr doc;
    xmlNodePtr root, cur_node;
    struct glbSet *set;
    struct glbRequestHandler *request = NULL;
    char *value = NULL;

    int ret = glb_OK;
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
    self->num_agents = 0;
    self->search_set_pool_size = 0;

    for (cur_node = root->children; cur_node; cur_node = cur_node->next) {
        if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"conc"))) {

	    value = (char *)xmlGetProp(cur_node,  (const xmlChar *)"n");
	    if (!value) goto error;

	    ret = glbMaze_add_search_term(self, value);
	    if (ret != glb_OK) goto error;
	    
	    xmlFree(value);
	    value = NULL;
	}
    }

    printf("    ++ NUM INTERSECTION SETS: %d\n", 
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

    if (value)
	xmlFree(value);

    if (request)
	request->del(request);

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
	printf("    !! Maze %d  got obj: %s\n %s\n", 
	       self->id, data->local_id, data->index);

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
    self->num_agents = 0;

    for (cur_node = root->children; cur_node; cur_node = cur_node->next) {
        if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"conc"))) {

	    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"n");
	    if (!value) continue;

	    ret = glbMaze_add_conc(self, 
				   NULL, 
				   cur_node->children, 
				   (const char*)value,
				   data->local_id);
	    if (ret != glb_OK) goto error;
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



    /* cache */
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


    glbMaze_init(self);

    *maze = self;

    return glb_OK;


error:
    glbMaze_del(self);
    
    return ret;
}
