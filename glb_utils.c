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
    
    if (i < 0) return glb_FAIL; 
    if (id[i] == '9') { id[i] = 'A'; return glb_OK;}
    if (id[i] == 'Z') { id[i] = 'a'; return glb_OK;}
    if (id[i] == 'z') { id[i--] = '0'; goto begin; }
    id[i]++;

    return glb_OK;
}

char *max_id(const char *a, const char *b)
{
    return compare(a, b) == glb_MORE ? a : b;
}
char *min_id(const char *a, const char *b)
{
    return compare(a, b) == glb_LESS ? a : b;
}

/*  intersects 2 id intervals and retruns 1 id interval
 *  return value: 1 to the left then 2
 *  less means left to the ... here
 */

int glb_intersection(const char *aid, const char *bid, const char *cid, const char *did, char *left, char *right)
{
    
    char *rleft;
    char *rright;
    int res;

    rleft = max_id(aid, cid);
    rright = min_id(bid, did);

    res = compare(rleft, rright); 
    
    if (res == glb_MORE) return glb_NOT_COMPARABLE;
    left = rleft;
    right = rright;
    return compare(did, bid) == glb_MORE ? glb_LESS : glb_MORE;
}




