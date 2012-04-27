#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glb_collection.h"
#include "glb_set.h"
#include "glb_index_tree.h"
#include "glb_set_file.h"
#include "glb_request_handler.h"
#include "glb_config.h"

#include "oodict.h"
#include "fcntl.h"

#define SET_COUNT 10

static int
glbCollection_tconcept_to_sets(struct glbCollection *self, char *bytecode, struct glbSet **set_pool)
{
    /* II. adding cur_id and locsets to sets */
    /* выделили сет с локсетом и сразу в Адъ*/

    return glb_OK;
}

static int
glbCollection_newDocument(struct glbCollection *self, struct glbAddRequest *request)
{
    int fd;
    size_t res;

    /*  I. transforming terminal concepts to set addresses*/
    
    self->tconcept_to_sets(self, request->concept_index, request->set_pool);

    /* III. saving doc */

    memcpy(self->key_id, self->cur_id, GLB_ID_MATRIX_DEPTH); /* adding '\0' */
    
    fd = open(self->key_id, O_RDWR);
    if (fd < 0) return glb_IO_FAIL;

    res = write(fd, request->source_string, request->source_string_size);
    
    close(fd);
    if (res <= request->source_string_size) 
        return glb_IO_FAIL; 

    inc_id(self->cur_id);     

    return glb_OK;
}

static int
glbCollection_addDocToSet(struct glbCollection *self, char *name)
{
    struct glbSet *set;

    self->findSet(self, name, &set);
    
    if (!set) return glb_OK;
    
    set->add(set, self->cur_id, "", 1 );

    return glb_OK;

}

static int
glbCollection_findDocuments(struct glbCollection *self, struct glbSearchRequest *request)
{
    int res;
    struct glbRequestHandler *request_handler;
    /*   I. transforming terminal concepts to set addresses */

    self->tconcept_transform(self, request->concept_index, request->set_pool);

    /*TODO: sort sets in setpool*/
    
    /*  II. intersecting sets */
    /* TODO: init request_handler */
    
intersection:

    request_handler->intersect(request_handler);
    
    /* III. intersecting result_table. if results too few goto II */
    
    /*res = request_handler->locset_intersection(request_handler);*/

    if ((res != glb_STOP) && (request_handler->result_size < request->result_size))
        goto intersection;

    /* IV. got answer*/

    return glb_OK;
}

static int
glbCollection_newSet(struct glbCollection *self, char *name, struct glbSet **answer)
{
    int result;

    struct glbSet *set;
    struct glbSetFile *file;
    struct glbIndexTree *index;

    if (GLB_COLLECTION_DEBUG_LEVEL_2)
        printf("adding new struct glbSet [%s]\n", name);
    
    result = glbSet_new(&set, name, "");
    if (result) return glb_NOMEM;
   
    if (self->dict->set(self->dict, name, set)) {
        set->del(set);
        return glb_NOMEM;
    }
	
    *answer = set;
    return glb_OK;
}

static int
glbCollection_findSet(struct glbCollection *self, char *name, struct glbSet **set)
{

    *set = self->dict->get(self->dict, name);
    return glb_OK;
}

static int
glbCollection_print(struct glbCollection *self)
{
    printf("<struct glbCollection at %p>\n", self);
    return glb_OK;
}

char* glbCollection_process(struct glbCollection *self, char *msg)
{
    struct glbSet **set_pool;
    size_t set_pool_blocks;
    size_t i, j, len;
    int result;
    char tmp[GLB_NAME_LENGTH];

    struct glbSet *set;

    set_pool_blocks = 1;
    set_pool = malloc(set_pool_blocks * SET_COUNT * sizeof(struct glbSet *));

    i = 0; j = 0;
    len = strlen(msg);
    while (i < len) {
        
        if (msg[i] == '#') {
            strncpy(tmp, msg + j, i - j);
            tmp[i - j] = '\0';
            printf("concept name: [%s]; ", tmp);
            set = NULL;
            self->findSet(self, tmp, &set);
            if (!set) {
                self->newSet(self, tmp, &set);
                printf("new set created;\n");
            } else printf("set allready exists;\n");

            j = i + 1;
        }
    
    
        i++;
    }

    return "OK";
}


static int
glbCollection_del(struct glbCollection *self)
{
    if (GLB_COLLECTION_DEBUG_LEVEL_1)
        printf("deleting struct glbCollection at %p...\n", self);

    if (!self) return glb_OK;
    
    if (self->dict) self->dict->del(self->dict);
    if (self->storage) self->storage->del(self->storage);

    free(self);
    return glb_OK;
}

static int
glbCollection_init(struct glbCollection *self)
{
    size_t count; /* max num of docs */
    size_t i;

    self->print = glbCollection_print;
    self->del = glbCollection_del;
    self->findSet = glbCollection_findSet; 
    self->newSet = glbCollection_newSet;
    self->newDocument = glbCollection_newDocument;
    self->findDocuments = glbCollection_findDocuments;
    self->addDocToSet = glbCollection_addDocToSet;

    self->id_pool[0][0] = '0'; 
    self->id_pool[0][1] = '0'; 
    self->id_pool[0][2] = '0'; 

    /* filling id_pool */

    count = GLB_RADIX_BASE * GLB_RADIX_BASE * GLB_RADIX_BASE;
    
    for (i = 1; i < count; i++) {
       memcpy(self->id_pool[i], self->id_pool[i - 1], GLB_ID_MATRIX_DEPTH);
       inc_id(self->id_pool[i]);
    }

    self->cur_id = self->id_pool[0];

    return glb_OK;
}

int glbCollection_new(struct glbCollection **rec)
{
    struct glbCollection *self;
    struct ooDict *dict, *storage;
    int result;
    size_t i, count; /* max num of docs */
    
    self = malloc(sizeof(struct glbCollection));
    if (!self) return glb_NOMEM;
    
/*	self->cur_id = malloc(GLB_ID_MATRIX_DEPTH * sizeof(char));
	if (!self->cur_id) return glb_NOMEM;
*/
    count = GLB_RADIX_BASE * GLB_RADIX_BASE * GLB_RADIX_BASE;
    
    self->id_pool = malloc(count * sizeof(char *));
	if (!self->id_pool) {
        glbCollection_del(self);
        return glb_NOMEM;
    }
  
    for (i = 0; i < count; i++) {
        self->id_pool[i] = malloc(GLB_ID_MATRIX_DEPTH * sizeof(char));
        if (!self->id_pool[i]) {
            glbCollection_del(self);
            return glb_NOMEM;
        }
    }

    result = ooDict_new(&dict);
    if (result) {
        glbCollection_del(self);
        return glb_FAIL;
    }
    self->dict = dict;
    
    result = ooDict_new(&storage);
    if (result) {
        glbCollection_del(self);
        return glb_FAIL;
    }
    self->storage = storage;

    glbCollection_init(self); 
    *rec = self;
    return glb_OK;
}
