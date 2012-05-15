#include <stdio.h>
#include <stdlib.h>

#include "glb_config.h"
#include "glb_utils.h"

int compare(const char *a, const char *b)
{
    int i;
    for (i = 0; i < GLB_ID_MATRIX_DEPTH; i++) {
        if (a[i] > b[i]) return glb_MORE;
        else if (a[i] < b[i]) return glb_LESS;
    }
    
    return glb_EQUALS;
}

int inc_id(char *id)
{
    size_t i;

    i = GLB_ID_MATRIX_DEPTH - 1;
    begin:
    
    if (id[i] == '9') { id[i] = 'A'; return glb_OK;}
    if (id[i] == 'Z') { id[i] = 'a'; return glb_OK;}
    if (id[i] == 'z') { id[i--] = '0'; goto begin; }
    id[i]++;

    return glb_OK;
}

const char *max_id(const char *a, const char *b)
{
    return (compare(a, b) == glb_MORE) ? a : b;
}

const char *min_id(const char *a, const char *b)
{
    return compare(a, b) == glb_LESS ? a : b;
}
