#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifndef _STACK_H_
#define _STACK_H_

#define STACK_STATE_LIST(X) \
    X(STACK_OK, "stack is ok") \
    X(STACK_EMPTY_ERROR, "stack is empty") \
    X(STACK_ALLOCATION_FAIL_ERROR, "memory allocation in stack has failed")

typedef enum {
#define X(name, value) name,
    STACK_STATE_LIST(X)
#undef X
} StackStatus;

typedef struct {
    int          index;
    int          bufsize;
    void        *buf;
    StackStatus status;
} Stack;

Stack *stack_init();
void stack_free(Stack *stack);
const char *stack_get_error_msg(Stack *stack);
void stack_push(Stack *stack, uint16_t val);
uint16_t stack_pop(Stack *stack);

#endif
