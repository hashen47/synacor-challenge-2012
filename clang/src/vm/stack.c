#include "../../include/stack.h"

static const char *stack_error_msgs[] = {
#define X(name, value) [name] = value,
    STACK_STATE_LIST(X)
#undef X
};

Stack *stack_init() {
    Stack* stack = (Stack*)malloc(sizeof(Stack));
    if (stack == NULL) {
        return NULL;
    }

    stack->index   = -1; 
    stack->bufsize = 2;
    stack->status  = STACK_OK;
    stack->buf     = NULL;

    stack->buf = malloc(sizeof(uint16_t) * stack->bufsize);
    if (stack->buf == NULL) {
        return NULL;
    }

    return stack;
}

void stack_free(Stack *stack) {
    if (stack == NULL) {
        return;
    }

    if (stack->buf != NULL) {
        free(stack->buf);
    }

    free(stack);
}

const char *stack_get_error_msg(Stack *stack) {
    switch (stack->status) {
#define X(name, value) case name: return stack_error_msgs[name];
        STACK_STATE_LIST(X)
#undef X
        default:
            return "undefined stack status value";
    }
}

void stack_push(Stack *stack, uint16_t val) {
    if (stack->status != STACK_OK) {
        return;
    }

    if (stack->index + 2 >= stack->bufsize) {
        stack->bufsize *= 2;
        void* newbuf = realloc(stack->buf, sizeof(uint16_t) * stack->bufsize); 
        if (newbuf == NULL) {
            stack->status = STACK_ALLOCATION_FAIL_ERROR;
            return;
        }
        stack->buf = newbuf;
    }

    *((uint16_t*)stack->buf + ++stack->index) = val;
}

uint16_t stack_pop(Stack *stack) {
    if (stack->status != STACK_OK) {
        return 0;
    }

    if (stack->index <= -1) {
        stack->status = STACK_EMPTY_ERROR;
        return 0;
    }

    return *((uint16_t*)stack->buf + stack->index--);
}
