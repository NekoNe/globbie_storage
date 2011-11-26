
#include <stdio.h>
#include <stdlib.h>

#include "glb_config.h"
#include "glb_locset.h"

enum { glb_EQUALS, glb_LESS, glb_MORE } glb_comparison_codes;

typedef struct glbSetElem
{
    char *id;
    struct glbLocSet *locset;
    
    /**********  public methods  **********/
    
    int (*init)(struct glbSetElem *self);
    int (*del)(struct glbSetElem *self);

    char (*str)(struct glbSetElem *self);

    int (*pack)(struct glbSetElem *self, char *buff, size_t *buff_size); /* struct to binary data */
    int (*unpack)(struct glbSetElem *self, char *buff); 

    int (*compare)(char *self, char *another); 

} glbSetElem;
