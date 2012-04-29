#include <stdio.h>
#include <stdlib.h>

#include "glb_config.h"
#include "glb_utils.h"


#define GLB_STACK_BLOCK_SIZE 10


struct glbStackElem
{
    char *record;
    /* previous stack top */ 
    struct glbStackElem *prev;

} glbStackElem;

struct glbStack
{
    struct glbStackElem *top;
    size_t depth;

    /********** public methods **********/

    int (*push)(struct glbStack *self, char *elem, size_t length);
    int (*pop)(struct glbStack *self, char **elem);

    int (*init)(struct glbStack *self);
    int (*del)(struct glbStack *self);

    /********** private methods **********/

    int (*malloc)(struct glbStack *self);

} glbStack;

extern int glbStack_new(struct glbStack **rec);

struct glbFunction
{
    size_t arg_count;

    int (*func)(void *control, struct glbStack *stack, size_t argc, char **argv);

} glbFunction;


struct glbInterpreter
{
    /* function names and its addresses */
    struct ooDict *functions; 
    struct glbStack *stack;

    void *control;

    /* result of tokeniztion */
    char **tokens; 

    /* keywords */
    char eol;
    char separator;
    char func_keyword;
    char separators[2];
    size_t sep_num;

    /* current position in tape */
    size_t marker; 

    /********** public methods **********/

    int (*interpret)(struct glbInterpreter *self, char *tape);

    int (*init)(struct glbInterpreter *self);
    int (*del)(struct glbInterpreter *self);

    /********** private methods **********/

    int (*next_token)(struct glbInterpreter *self, char *tape, size_t *pozition, size_t *length);
    int (*is_separator)(struct glbInterpreter *self, char ch);

} glbInterpreter;

extern int glbInterpreter_new(struct glbInterpreter **rec);
