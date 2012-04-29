#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glb_interpreter.h"
#include "oodict.h"


static int
glbStack_pop(struct glbStack *self, char **elem)
{
    size_t length;
    struct glbStackElem *tmp;
    char *string;

    if (!self->top) return glb_EOB;

    length = strlen(self->top->record);
    

    string = malloc((length + 1) * sizeof(char));
    if (!string) return glb_EOB;
    
    memcpy(string, self->top->record, length + 1);
    
    *elem = string;

    tmp = self->top;
    self->top = self->top->prev;
    if (tmp->record) free(tmp->record);
    free(tmp);

    return glb_OK;

}

static int
glbStack_push(struct glbStack *self, char *elem, size_t length)
{
    struct glbStackElem *tmp;

    size_t i;
  
    
    tmp = malloc(sizeof(struct glbStackElem));
    if (!tmp) return glb_NOMEM;
    
    tmp->prev = self->top; 
    self->top = tmp;
    
    self->top->record = malloc((length + 1) * sizeof(char));
    if (!self->top->record) return glb_NOMEM;

    memcpy(self->top->record, elem, length);
    self->top->record[length] = '\0';
    
    return glb_OK;
}

static int
glbStack_del(struct glbStack *self)
{
    return glb_OK;
}

static int 
glbStack_init(struct glbStack *self)
{
    self->init = glbStack_init;
    self->del = glbStack_del;
    self->push = glbStack_push;
    self->pop = glbStack_pop;

    self->top = NULL;
    self->depth = 0;

    return glb_OK;
}

int glbStack_new(struct glbStack **rec)
{
    struct glbStack *self;
    int result;

    self = malloc(sizeof(struct glbStack));
    if (!self) return glb_NOMEM;
    
    
    result = glbStack_init(self);
    if (result) goto error;
    *rec = self;
    return glb_OK;

error:
    glbStack_del(self);
    return result;

}

static int 
glbInterpreter_interpret(struct glbInterpreter *self, char *tape)
{
    int result, i;
    size_t position, length; /* of token */
    struct glbStack *stack;
    char **argv; /* function args */
    char *tmp;
    char func_name[GLB_NAME_LENGTH]; 
    struct glbFunction *func;


    stack = self->stack;

    result = glbStack_new(&stack);
    if (result) return result;

    while (1) {
        result = self->next_token(self, tape, &position, &length);
        if (result == glb_EOB) break;

        printf("    token got: [");
        for (i = 0; i < length; i++) {
            printf("%c", tape[position + i]);
        }
        printf("];\n");

        if (tape[position] != self->func_keyword) {
            result = stack->push(stack, tape + position, length);
            continue;
        }
       
        /* getting function name and finding function*/
        func = NULL;
        memcpy(func_name, tape + position + 1, length - 1);
        func_name[length - 1] = '\0';
        func = self->functions->get(self->functions, func_name);
        
        if (!func) { 
            printf("function not found\n");
            return glb_FAIL;
        }
        
        /* preparing args */
        argv = malloc(func->arg_count * sizeof(char *));
        
        for (i = 0; i < func->arg_count; i++) {
            result = stack->pop(stack, &tmp);
            
            if (result) {
                printf("error: not enough args in stack\n");
                /* TODO: make function of this */
                if (argv) {
                    for (i = 0; i < func->arg_count; i++) 
                        if (argv[i]) free(argv[i]);
                    free(argv);
                }
                return glb_FAIL;
            }
            argv[i] = tmp;
        }
        func->func(stack, func->arg_count, argv); 
        
        if (argv) {
            for (i = 0; i < func->arg_count; i++) 
                if (argv[i]) free(argv[i]);
            free(argv);
        }
    }
    return glb_OK;
}

static int
glbInterpreter_next_token(struct glbInterpreter *self, char *tape, size_t *position, size_t *length)
{
    size_t i, begin;

    i = self->marker;
    
    /* finding the begining of a token */
    //while (tape[i] == self->separator) i++;
    while (self->is_separator(self, tape[i])) i++;
    if (tape[i] == self->eol) return glb_EOB;
    begin = i;

    /* finding the end of a token */
    while ((!(self->is_separator(self, tape[i]))) && (tape[i] != self->eol)) i++;
    *position = begin;
    *length = i - begin;
    self->marker = i;

    return glb_OK;
}

static int 
glbInterpreter_is_separator(struct glbInterpreter *self, char ch)
{
    size_t i;
    for (i = 0; i < self->sep_num; i++) {
        if (ch == self->separators[i]) return true;
    }
    return false;
}

static int
glbInterpreter_del(struct glbInterpreter *self)
{
    /* TODO */
    return glb_OK; 
}

static int
glbInterpreter_init(struct glbInterpreter *self)
{
    self->init = glbInterpreter_init;
    self->del = glbInterpreter_del;
    self->interpret = glbInterpreter_interpret;
    self->next_token = glbInterpreter_next_token;
    self->is_separator = glbInterpreter_is_separator;

    self->marker = 0;
    self->separator = ' ';
    
    self->separators[0] = ' ';
    self->separators[1] = '\n';
    self->sep_num = 2;

    self->func_keyword = '@';
    self->eol = '\0';

    return glb_OK;
}

int glbInterpreter_new(struct glbInterpreter **rec)
{
    struct glbInterpreter *self;
    struct ooDict *func;
    int result;

    self = malloc(sizeof(struct glbInterpreter));
    if (!self) return glb_NOMEM;
    

    result = ooDict_new(&func);
    if (result) goto error;
    self->functions = func;


    result = glbInterpreter_init(self);
    if (result) goto error;
    *rec = self;
    return glb_OK;

error:
    glbInterpreter_del(self);
    return result;
}
