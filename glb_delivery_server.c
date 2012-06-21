#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include <libxml/parser.h>

#include <zmq.h>
#include "zhelpers.h"

#include "glb_config.h"
#include "glb_utils.h"
#include "glb_delivery.h"


int 
main(int const argc, 
     const char ** const argv) 
{
    struct glbDelivery *delivery;
    const char *config = "delivery.ini";
    int ret;
    
    xmlInitParser();

    ret = glbDelivery_new(&delivery, config);
    if (ret) {
        fprintf(stderr, "Couldn\'t load glbDelivery... ");
        return -1;
    }

    delivery->name = "GLB DELIVERY SERVICE";

    delivery->start(delivery);

    return 0;
}


