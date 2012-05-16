#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

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


static int 
glb_mkdir(const char *path, mode_t mode)
{
    struct stat st;
    int ret = glb_OK;

    if (stat(path, &st) != 0) {

        /* directory does not exist */
        if (mkdir(path, mode) != 0)
            ret = glb_FAIL;
    }
    else if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        ret = glb_FAIL;
    }

    return ret;
}


/**
 * glb_mkpath - ensure all directories in path exist
 */
int glb_mkpath(const char *path, mode_t mode)
{
    char *p;
    char *sep;
    int  ret;
    char *path_buf = strdup(path);

    ret = glb_OK;
    p = path_buf;

    while (ret == glb_OK && 
           (sep = strchr(p, '/')) != 0) {
	if (sep != p) {
            *sep = '\0';
            ret = glb_mkdir(path_buf, mode);
            *sep = '/';
        }
        p = sep + 1;
    }

    /* in case no final dir separator is present at the end */
    if (ret == glb_OK)
        ret = glb_mkdir(path, mode);

    free(path_buf);

    return ret;
}
