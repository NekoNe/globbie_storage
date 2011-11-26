#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#include "glb_config.h"
#include "glb_set.h"
#include "glb_utils.h"

#define ID_COUNT 200000 

int main(int argc, char *argv[])
{
    size_t i, j;
    size_t ls;
    int fd;
    char *env_path = "tmp";
    char *path_set = "set";
    char *path_locset = "locset";
    char info;
    char **ids;
    const char iid[3] = {'z', 'z', 'z'}; 
    char *locset = NULL;
    clock_t timer;


    /* Preparing data structures */  
    struct glbSet *adam;
    struct glbSetFile *adamsdata;
    struct glbIndexTree *adamsindex;
    struct glbIndexTreeNode *itn;    

    info = sizeof(size_t);
    /** Memory allocation **/
    
    ids = malloc(sizeof(char *) * ID_COUNT);
    if (!ids) return glb_FAIL;

    for (j = 0; j < ID_COUNT; j++) {
        ids[j] = malloc(GLB_ID_MATRIX_DEPTH * sizeof(char));
        if (!ids[j]) return glb_FAIL;
    }
    
    printf("Memory was allocated!\n");

    /** Preparing ids **/

    ids[0][0] = '0';
    ids[0][1] = '0';
    ids[0][2] = '0';

    for (j = 1; j < ID_COUNT; j++) {
        memcpy(ids[j], ids[j-1], GLB_ID_MATRIX_DEPTH);
        inc_id(ids[j]);
      /*  printf("%c%c%c\n",ids[j][0], ids[j][1], ids[j][2]);*/
    }
    
    /* files creation */

    fd = open(path_set, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    close(fd);
    fd = open(path_locset, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    close(fd);
    
    /******* ADAM'S INITIALISATION ******/
    glbSet_new(&adam);
    glbSetFile_new(&adamsdata);
    glbIndexTree_new(&adamsindex);

    adam->data = adamsdata;
    adam->index = adamsindex;
   
    adam->env_path = env_path;
    adam->data->path_set = path_set;
    adam->data->path_locset = path_locset;


    fd = open(adam->data->path_set, O_WRONLY | O_TRUNC | O_CREAT);
    if (fd < 0) return glb_IO_FAIL;
    printf("%i\n", write(fd, &info, 1));
    close(fd);
    fd = open(adam->data->path_locset, O_WRONLY | O_TRUNC | O_CREAT);
    if (fd < 0) return glb_IO_FAIL;
    write(fd, &info, 1);
    close(fd);
    printf("Structs was created\n");
    /* files creation */
    /*
    fd = open(path_set, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    close(fd);
    fd = open(path_locset, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    close(fd);
    printf("Structs was created\n");
    */
    /****** ADAM'S TEST *****/
    /** Test 1: adding **/

    for (j = 0; j < ID_COUNT; j++) {
        /*printf("Iterration number %i\n", j);*/
        adam->add(adam, ids[j], locset, 0);
    }
    /*adam->add(adam, id1, locset, 0);*/    
    printf("ID's was added in set\n");

    /* speed test */
    timer = clock();

    for (j = 0; j < ID_COUNT; j+=7) {
        /*printf("lookup error code: %d\n",*/ adam->lookup(adam, ids[j], &ls)/*)*/;
    }
    timer = clock() - timer;
    printf("Time elapsed: %u\n", (unsigned int)timer);


   /* for (j = 0; j < 100; j++ ) {
        printf("%i ", adam->index->array[j].offset);
    }*/

    /*
    itn = adam->index->root;
    for (j = 0; j < 100; j++) {
        if (!itn) continue;
        printf("%i %c%c%c  \n", itn->offset, itn->id[0],itn->id[1],itn->id[2]);
        itn = itn->left;
    }
    */

    return glb_OK;
}

