#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>


#include <zmq.h>
#include "zhelpers.h"

#include "glb_config.h"
#include "glb_storage.h"

int 
main(int           const argc, 
     const char ** const argv) 
{
    struct glbStorage *storage;
    const char *config = "storage.ini";
    int ret;

    ret = glbStorage_new(&storage, config);
    if (ret) {
        fprintf(stderr, "Couldn\'t load glbStorage... ");
        return -1;
    }

    storage->start(storage);

    return 0;
}


