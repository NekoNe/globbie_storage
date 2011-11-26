
#include <stdio.h>
#include <stdlib.h>

#include "glb_config.h"
#include "glb_set.h"


/* lookup in index and get offset in set
 * lookup in set and get offset in locsets
 * write locset's offset to *lockset_pointer 
 */
static int 
glbSet_lookup(struct glbSet *self, const char *id, size_t *locset_pointer)
{
    size_t offset;
    int res;

    if (DEBUG_LEVEL_3) 
        printf("Looking up id \"%c%c%c\" in the index...\n", id[0], id[1], id[2]);

    res = self->index->lookup(self->index, id, &offset, self->index->root); 
    if (res == glb_NO_RESULTS) return glb_NO_RESULTS;

    if (DEBUG_LEVEL_3) 
        printf("Reading offset \"%d\" from the data file...\n", offset);
   
    res = self->data->lookup(self->data, id, &offset);
    if (res == glb_NO_RESULTS) return glb_NO_RESULTS;
    *locset_pointer = offset;

    if (DEBUG_LEVEL_3)
        printf("Lookup success! Offset: %d\n", *locset_pointer);

    return glb_OK;
}

/*  Add to locset file
 *  Add to set file
 *  Add to index
 *  TODO: if any of these steps fail the function must discard the changes
 */
static int
glbSet_add(struct glbSet *self, const char *id, const char *bytecode, size_t bytecode_size)
{
    int state;
    size_t offset;
    
    state = self->data->addByteCode(self->data, bytecode, bytecode_size, &offset);
    state = self->data->addId(self->data, id, offset, &offset);
        
    if (self->index_counter % GLB_LEAF_SIZE == 0) {
        state = self->index->addElem(self->index, id, offset);
        state = self->index->update(self->index, self->index->root);
        self->index_counter = 0;
    }
    self->index_counter += 1;

    return glb_OK;
}

/* Берем 1-ые листья деревьев (в лин. порядке)
 * Из них определяем MaxOffset
 * По 2-ым листьям определяем MinOffset 
 * Запоминаем, кто владеет индексом с MinOffset (назову это левым листом)
 * На отрезке [MaxOffset, MinOffset] буфферезуем файлы set'ов
 * Пересекаем их. Записываем в новый set
 * 
 * В сохраненном set'е сдвигаем указатель на лист на 1 в право.
 * Всё это повторяется до тех пор пока не найдется лист, указатель на который
 * нельзя переместить правее.
 * 
 * Пока буду сохранять результат не в новый сет, а в участок памяти. Оффсеты игнорирую 
 * до создания коллекции.
 */
static int
glbSet_intersect(struct glbSet *self, struct glbSet **setpool, size_t count, struct glbSet *result)
{
    size_t left_leaf_owner;
    



    return glb_OK;
}


static int 
glbSet_del(struct glbSet *self)
{
    /* TODO */
    return glb_OK;
}

static int
glbSet_init(struct glbSet *self)
{
    self->init = glbSet_init;
    self->del = glbSet_del;
    self->add = glbSet_add;
    self->lookup = glbSet_lookup;
    self->intersect = glbSet_intersect;

    self->data = NULL;
    self->index = NULL;

    self->index_counter = 0;

    return glb_OK;
}

int glbSet_new(struct glbSet **rec)
{
    struct glbSet *self;

    self = malloc(sizeof(struct glbSet));
    if (!self) return glb_NOMEM;
        
    glbSet_init(self);

    *rec = self;
    return glb_OK;
}
