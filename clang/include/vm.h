#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "stack.h"

#ifndef _VM_H_
#define _VM_H_

#define MEM_SIZE  32768
#define REG_COUNT 8 
#define MODULO  32768 
#define NO_REG  8           // NOTE: reg count is 8, then index cannot be 8
#define NO_NUM  UINT16_MAX  // NOTE: all numbers below 32775, so this is ok

#define VM_STATE_LIST(X) \
    X(VM_OK, "vm is ok") \
    X(VM_STACK_INIT_FAIL_ERROR, "stack initialization fail in vm (check the stack error if stack exists)") \
    X(VM_STACK_PUSH_FAIL_ERROR, "stack push failed in vm") \
    X(VM_STACK_POP_FAIL_ERROR, "stack pop failed in vm") \
    X(VM_INVALID_INSTRUCTION_ERROR, "invalid instruction for vm") \
    X(VM_LOAD_BINARY_FAIL_ERROR, "load binary has failed in vm") \
    X(VM_MEMORY_OVERFLOW_ERROR, "memory overflow happend in vm") \
    X(VM_INVALID_REG_ERROR, "invalid register index in vm") \
    X(VM_INVALID_NUM_ERROR, "invalid number value in vm")

typedef enum {
#define X(name, value) name,
    VM_STATE_LIST(X)
#undef X
} VM_Status;

typedef struct {
    bool      halt;
    bool      should_skip_on_reg_or_num_err; // NOTE: if this is true when reg or num error occur then it skip that error and update the *pos* to read next instruction
    VM_Status status;
    uint16_t  mem[MEM_SIZE];
    uint16_t  regs[REG_COUNT];
    Stack     *stack;
    uint16_t  pos;
} VM;

VM *vm_init(bool should_skip_on_reg_or_num_err);
void vm_free(VM *vm);
const char *vm_get_error_msg(VM *vm);
void vm_reset(VM *vm);
void vm_load_binary(VM *vm, const char* path);
void vm_print_memory(VM *vm);
void vm_load_test(VM *vm);
uint16_t vm_get_reg(uint16_t n);
uint16_t vm_get_num(VM* vm, uint16_t n);
void vm_next_inst(VM *vm);
void vm_process(VM *vm);

#endif
