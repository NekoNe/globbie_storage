#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "oodict.h"
#include "glb_maze.h"
#include "glb_set.h"
#include "glb_index_tree.h"
#include "glb_set_file.h"
#include "glb_request_handler.h"
#include "glb_config.h"
#include "glb_interpreter.h"
#include "glb_collection.h"

#define SET_COUNT 10

/* declarations */
static int
glbColl_findSet(struct glbColl *self, char *name, struct glbSet **set);

static int
glbColl_tconcept_to_sets(struct glbColl *self, char *bytecode, struct glbSet **set_pool)
{
    /* II. adding cur_id and locsets to sets */
    /* выделили сет с локсетом и сразу в Адъ*/

    return glb_OK;
}

static int
glbColl_add(struct glbColl *self,
	    const char *spec,
	    const char *obj)
{
    int fd;
    size_t res;

    /*  I. transforming terminal concepts to set addresses*/
    
    /*glbColl_tconcept_to_sets(self, request->concept_index, request->set_pool);*/


    /* save original object */

    memcpy(self->key_id, self->cur_id, GLB_ID_MATRIX_DEPTH); /* adding '\0' */
    
    /*fd = open(self->key_id, O_RDWR);
    if (fd < 0) return glb_IO_FAIL;

    res = write(fd, request->source_string, request->source_string_size);
    
    close(fd);
    if (res <= request->source_string_size) 
        return glb_IO_FAIL; 

    inc_id(self->cur_id);     
    */


    return glb_OK;
}

static int
glbColl_addDocToSet(struct glbColl *self, char *name)
{
    struct glbSet *set;

    set = NULL;
    glbColl_findSet(self, name, &set);
    
    if (!set) return glb_OK;
    
    set->add(set, self->cur_id, "", 1 );

    return glb_OK;

}

static int
glbColl_find(struct glbColl *self, 
	     const char *spec,
	     const char *request)
{
    int res;
    struct glbRequestHandler *request_handler;

    /*   I. transforming terminal concepts to set addresses */

    /* glbColl_tconcept_transform(self, request->concept_index, request->set_pool); */

    /*TODO: sort sets in setpool*/
    
    /*  II. intersecting sets */
    /* TODO: init request_handler */
    
    /*intersection:

    request_handler->intersect(request_handler);
    */

    /* III. intersecting result_table. if results too few goto II */
    
    /*res = request_handler->locset_intersection(request_handler);*/

    /*    if ((res != glb_STOP) && (request_handler->result_size < request->result_size))
        goto intersection;
    */

    /* IV. got answer*/

    return glb_OK;
}

static int
glbColl_newSet(struct glbColl *self, char *name, struct glbSet **answer)
{
    int result;

    struct glbSet *set;
    struct glbSetFile *file;
    struct glbIndexTree *index;

    if (GLB_COLLECTION_DEBUG_LEVEL_2)
        printf("adding new struct glbSet [%s]\n", name);
    
    result = glbSet_new(&set, name, "");
    if (result) return glb_NOMEM;
   
    if (self->set_index->set(self->set_index, name, set)) {
        set->del(set);
        return glb_NOMEM;
    }
	
    *answer = set;
    return glb_OK;
}

static int
glbColl_findSet(struct glbColl *self, char *name, struct glbSet **set)
{
    *set = self->set_index->get(self->set_index, name);
    return glb_OK;
}

static int
glbColl_str(struct glbColl *self)
{
    printf("<struct glbColl at %p>\n", self);

    return glb_OK;
}

static int
glbColl_addDoc(void *control, struct glbStack *stack, size_t argc, char **argv)
{
    size_t i, len, begin;
    struct glbColl *self;
    struct glbSet *set;
    char name[GLB_NAME_LENGTH];

    set = NULL;
    self = (struct glbColl *)control;
    printf("in add doc\n"); 
    
    /* parse first arg */
    len = strlen(argv[0]);
    
    printf("in addDoc parsing [%s], len %d\n", argv[0], len);
    
    begin = 0;
    i = 0;
    while (i < len) {
        if (argv[0][i] != '#') { i++; continue; }
        
        memcpy(name, argv[0] + begin, i - begin);
        name[i - begin] = '\0';
        begin = i + 1;
        printf("%s\n", name);
        
        i++;
    
        /* finding concept */
        set = NULL;
        glbColl_findSet(self, name, &set);     
        
        if (!set)    
            glbColl_newSet(self, name, &set);

        self->cur_id = self->id_pool[self->id_count];
        
        set->add(set, self->cur_id, "", 1 );
    }
                   
        self->id_count++;
    return glb_OK;
}

static int
glbColl_findDocs(void *control, struct glbStack *stack, size_t argc, char **argv)
{
    struct glbSet **set_pool;
    struct glbSet *set;
    char name[GLB_NAME_LENGTH];
    size_t i, begin, count, len;
    struct glbRequestHandler *request;
    struct glbColl *self;
    
    size_t answer_size;

    set = NULL;
    self = (struct glbColl *)control;
    
    /* parse first arg */
    len = strlen(argv[0]);
    count = 0; 
    set_pool = NULL;
    begin = 0;
    i = 0;

    while (i < len) {
        if (argv[0][i] != '#') { i++; continue; }
        
        memcpy(name, argv[0] + begin, i - begin);
        name[i - begin] = '\0';
        begin = i + 1;
        printf("concept: %s\n", name);
        
        i++;
    
        /* finding concept */
        
        set = NULL;
        glbColl_findSet(self, name, &set);     
        
        if (!set) {
            printf("no such concept\n");
            return glb_FAIL;
        }
        count++;
        set_pool = realloc(set_pool, count * sizeof(struct glbSet *));
        
        printf("count %d, %p\n",count, set_pool);
        set_pool[count - 1] = set; 
    }
     
    printf("set_pool ready, %d\n", count); 

    for (i = 0; i < count; i++) {
        printf("set[%d] in <%p>\n", i, set_pool[i]);
    }

    answer_size = atoi(argv[1]);
    printf("answer size %d\n", answer_size);
    glbRequestHandler_new(&request, answer_size, 0, set_pool, count);
    request->intersect(request);
    printf("result:\n");
    for (i = 0; i < request->result->answer_actual_size * GLB_ID_MATRIX_DEPTH; i++) {
              printf("%c", request->result->ids[i]);
    }

    //request->del(request);
    
}

const char* glbColl_process(struct glbColl *self, 
			    const char *msg)
{
    struct glbInterpreter *interpreter;
    struct glbFunction addDoc, findDocs;

    int result;

    result = glbInterpreter_new(&interpreter);
    if (result) return "glb internal error";
   
    interpreter->control = (void *)self;

    addDoc.arg_count = 1;
    addDoc.func = glbColl_addDoc;

    findDocs.arg_count = 2;
    findDocs.func = glbColl_findDocs;

    interpreter->functions->set(interpreter->functions, "addDoc", &addDoc);
    interpreter->functions->set(interpreter->functions, "findDocs", &findDocs);

    result = interpreter->interpret(interpreter, msg);
    if (result) return "interpretation error";
    
    return "OK";
}


static int
glbColl_del(struct glbColl *self)
{
    if (GLB_COLLECTION_DEBUG_LEVEL_1)
        printf("deleting struct glbColl at %p...\n", self);

    if (!self) return glb_OK;
    
    if (self->maze) self->maze->del(self->maze);
    if (self->set_index) self->set_index->del(self->set_index);
    if (self->storage) self->storage->del(self->storage);

    free(self);
    return glb_OK;
}

static int
glbColl_init(struct glbColl *self)
{
    size_t count; /* max num of docs */
    size_t i;

    self->str = glbColl_str;
    self->del = glbColl_del;

    self->add = glbColl_add;
    self->find = glbColl_find;

    /* first id */
    self->id_pool[0][0] = '0'; 
    self->id_pool[0][1] = '0'; 
    self->id_pool[0][2] = '0'; 
    self->id_count = 0;

    /* generate all ids */
    count = GLB_RADIX_BASE * GLB_RADIX_BASE * GLB_RADIX_BASE;
    for (i = 1; i < count; i++) {
       memcpy(self->id_pool[i], self->id_pool[i - 1], GLB_ID_MATRIX_DEPTH);
       inc_id(self->id_pool[i]);
    }

    self->cur_id = self->id_pool[0];

    return glb_OK;
}

int glbColl_new(struct glbColl **rec)
{
    struct glbColl *self;
    struct ooDict *dict, *storage;
    int result;
    size_t i, count; /* max num of docs */
    
    self = malloc(sizeof(struct glbColl));
    if (!self) return glb_NOMEM;

/*	self->cur_id = malloc(GLB_ID_MATRIX_DEPTH * sizeof(char));
	if (!self->cur_id) return glb_NOMEM;
*/
    count = GLB_RADIX_BASE * GLB_RADIX_BASE * GLB_RADIX_BASE;
    
    self->id_pool = malloc(count * sizeof(char *));
	if (!self->id_pool) {
        glbColl_del(self);
        return glb_NOMEM;
    }
  
    for (i = 0; i < count; i++) {
        self->id_pool[i] = malloc(GLB_ID_MATRIX_DEPTH * sizeof(char));
        if (!self->id_pool[i]) {
            glbColl_del(self);
            return glb_NOMEM;
        }
    }

    result = ooDict_new(&self->set_index, GLB_LARGE_DICT_SIZE);
    if (result) {
        glbColl_del(self);
        return glb_FAIL;
    }
 
    result = ooDict_new(&self->storage, GLB_LARGE_DICT_SIZE);
    if (result) {
        glbColl_del(self);
        return glb_FAIL;
    }

    glbColl_init(self); 
    *rec = self;
    return glb_OK;
}
