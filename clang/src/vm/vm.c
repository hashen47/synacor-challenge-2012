#include "../../include/vm.h"
#include "../../include/stack.h"

static const char *vm_error_msgs[] = {
#define X(name, value) [name] = value,
    VM_STATE_LIST(X)
#undef X
};

VM *vm_init(bool should_skip_on_reg_or_num_err) {
    VM* vm = (VM*)malloc(sizeof(VM));
    if (vm == NULL) {
        return NULL;
    }

    vm->stack = stack_init();
    if (vm->stack == NULL || vm->stack->status != STACK_OK) {
        vm->status = VM_STACK_INIT_FAIL_ERROR;
        vm_free(vm);
        return NULL;
    }

    vm->should_skip_on_reg_or_num_err = should_skip_on_reg_or_num_err;
    vm->status = VM_OK;
    vm_reset(vm);

    return vm;
}

void vm_free(VM *vm) {
    if (vm == NULL) {
        return;
    }

    if (vm->stack != NULL) {
        stack_free(vm->stack);
    }

    free(vm);
}

const char *vm_get_error_msg(VM *vm) {
    switch (vm->status) {
#define X(name, value) case name: return vm_error_msgs[name];
        VM_STATE_LIST(X)
#undef X
        default:
            return "undefined vm status value";
    }
}

void vm_reset(VM *vm) {
    if (vm->status != VM_OK) {
        return;
    }

    vm->halt = false;
    vm->pos  = 0;

    for (int i = 0; i < MEM_SIZE; i++) {
        vm->mem[i] = 0;
    }

    for (int i = 0; i < REG_COUNT; i++) {
        vm->regs[i] = 0;
    }
}

void vm_load_binary(VM* vm, const char* path) {
    if (vm->status != VM_OK) {
        return;
    }

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        char msg[100];
        snprintf(msg, sizeof(msg), "binary file path: %s", path);
        perror(msg);
        vm->status = VM_LOAD_BINARY_FAIL_ERROR;
        return;
    }

    int i = 0;
    int arr_i      = 0;
    uint8_t arr[2] = {0}; // store little endian unsigned 16 bit value

    int ch;
    while ((ch = fgetc(fp)) != EOF) {
        if (i >= MEM_SIZE) {
            vm->status = VM_MEMORY_OVERFLOW_ERROR;
            break;
        }
        arr[arr_i++] = (uint8_t)ch;
        if (arr_i == 2) {
            vm->mem[i++] = (uint16_t)(arr[0] | (arr[1] << 8));
            arr_i        = 0;
        }
    }

    fclose(fp);
}

void vm_print_memory(VM *vm) {
    if (vm->status != VM_OK) {
        return;
    }

    for (int i = 0; i < MEM_SIZE; i++) {
        printf("i: (%5d), val: %d\n", i, vm->mem[i]);
    }
}

void vm_load_test(VM* vm) {
    if (vm->status != VM_OK) {
        return;
    }

    vm_reset(vm);

    // 9,32768,32769,4,19,32768
    vm->mem[0]  = 9;
    vm->mem[1]  = 32768;
    vm->mem[2]  = 32769;
    vm->mem[3]  = 4;
    vm->mem[4]  = 19;
    vm->mem[5]  = 32768;
    vm->regs[1] = (uint16_t)'A';
}

uint16_t vm_get_reg(uint16_t n) {
    if (n < 32768 || n > 32775) {
        return NO_REG;
    }
    return n % MODULO;
}

uint16_t vm_get_num(VM* vm, uint16_t n) {
    uint16_t reg = vm_get_reg(n);
    if (n > 32767) {
        if (reg == NO_REG) {
            return NO_NUM;
        }
        return vm->regs[reg];
    }
    return n; 
}

void vm_process(VM *vm) {
    if (vm->status != VM_OK) {
        return;
    }

    while (vm->status == VM_OK && !vm->halt && vm->pos < MEM_SIZE) {
        // printf("pos: %d, instruction: %d\n", vm->pos, vm->mem[vm->pos]);
        vm_next_inst(vm);
    }
}

void vm_next_inst(VM* vm) {
    if (vm->status != VM_OK) {
        return;
    }

    uint16_t reg, a, b, c, val;
    switch (vm->mem[vm->pos]) {
        case 0: // halt
            vm->halt = true;
            break;
        case 1: // set
            reg = vm_get_reg(vm->mem[vm->pos + 1]);
            b   = vm_get_num(vm, vm->mem[vm->pos + 2]);

            if (reg == NO_REG) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_REG_ERROR;
                }
                return;
            }

            if (b == NO_NUM) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_NUM_ERROR;
                } else {
                    vm->pos += 3;
                }
                return;
            }

            vm->regs[reg] = b;
            vm->pos += 3;
            break;
        case 2: // push
            a = vm_get_num(vm, vm->mem[vm->pos + 1]);

            if (a == NO_NUM) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_NUM_ERROR;
                } else {
                    vm->pos += 2;
                }
                return;
            }

            stack_push(vm->stack, a);
            if (vm->stack->status != STACK_OK) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->status = VM_STACK_PUSH_FAIL_ERROR;
                    vm->halt = true;
                }
                return;
            }

            vm->pos += 2;
            break;
        case 3: // pop
            reg = vm_get_reg(vm->mem[vm->pos + 1]);

            if (reg == NO_REG) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_REG_ERROR;
                } else {
                    vm->pos += 2;
                }
                return;
            }

            val = stack_pop(vm->stack);
            if (vm->stack->status != STACK_OK) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_STACK_POP_FAIL_ERROR;
                }
                return;
            }

            vm->regs[reg] = val;
            vm->pos += 2;
            break;
        case 4: // eq
            reg = vm_get_reg(vm->mem[vm->pos + 1]); 
            b   = vm_get_num(vm, vm->mem[vm->pos + 2]);
            c   = vm_get_num(vm, vm->mem[vm->pos + 3]);

            if (reg == NO_REG) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_REG_ERROR;
                } else {
                    vm->pos += 4;
                }
                return;
            }

            if (b == NO_NUM || c == NO_NUM) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_NUM_ERROR;
                } else {
                    vm->pos += 4;
                }
                return;
            }

            vm->regs[reg] = 0;
            if (b == c) {
                vm->regs[reg] = 1;
            }

            vm->pos += 4;
            break;
        case 5: // gt 
            reg = vm_get_reg(vm->mem[vm->pos + 1]); 
            b   = vm_get_num(vm, vm->mem[vm->pos + 2]);
            c   = vm_get_num(vm, vm->mem[vm->pos + 3]);

            if (reg == NO_REG) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_REG_ERROR;
                }
                return;
            }

            if (b == NO_NUM || c == NO_NUM) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_NUM_ERROR;
                } else {
                    vm->pos += 4;
                }
                return;
            }

            vm->regs[reg] = 0;
            if (b > c) {
                vm->regs[reg] = 1;
            }

            vm->pos += 4;
            break;
        case 6: // jmp
            a = vm_get_num(vm, vm->mem[vm->pos + 1]);

            if (a == NO_NUM) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_NUM_ERROR;
                } else {
                    vm->pos += 3;
                }
                return;
            }

            vm->pos = a;
            break;
        case 7: // jt
            a = vm_get_num(vm, vm->mem[vm->pos + 1]);
            b = vm_get_num(vm, vm->mem[vm->pos + 2]);

            if (a == NO_NUM || b == NO_NUM) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_NUM_ERROR;
                } else {
                    vm->pos += 3;
                }
                return;
            }

            if (a == 0) {
                vm->pos += 3;
                return;
            }

            vm->pos = b;
            break;
        case 8: // jf
            a = vm_get_num(vm, vm->mem[vm->pos + 1]);
            b = vm_get_num(vm, vm->mem[vm->pos + 2]);

            if (a == NO_NUM || b == NO_NUM) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_NUM_ERROR;
                } else {
                    vm->pos += 3;
                }
                return;
            }

            if (a != 0) {
                vm->pos += 3;
                return;
            }

            vm->pos = b;
            break;
        case 9: // add
            reg = vm_get_reg(vm->mem[vm->pos + 1]);
            b   = vm_get_num(vm, vm->mem[vm->pos + 2]);
            c   = vm_get_num(vm, vm->mem[vm->pos + 3]);

            if (reg == NO_REG) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_REG_ERROR;
                } else {
                    vm->pos += 4;
                }
                return;
            }

            if (b == NO_NUM || c == NO_NUM) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_NUM_ERROR;
                }
                return;
            }

            vm->regs[reg] = (b + c) % MODULO;
            vm->pos += 4;
            break;
        case 10: // mult
            reg = vm_get_reg(vm->mem[vm->pos + 1]);
            b   = vm_get_num(vm, vm->mem[vm->pos + 2]);
            c   = vm_get_num(vm, vm->mem[vm->pos + 3]);

            if (reg == NO_REG) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_REG_ERROR;
                } else {
                    vm->pos += 4;
                }
                return;
            }

            if (b == NO_NUM || c == NO_NUM) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_NUM_ERROR;
                } else {
                    vm->pos += 4;
                }
                return;
            }

            vm->regs[reg] = (b * c) % MODULO;
            vm->pos += 4;
            break;
        case 11: // mod 
            reg = vm_get_reg(vm->mem[vm->pos + 1]);
            b   = vm_get_num(vm, vm->mem[vm->pos + 2]);
            c   = vm_get_num(vm, vm->mem[vm->pos + 3]);

            if (reg == NO_REG) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_REG_ERROR;
                } else {
                    vm->pos += 4;
                }
                return;
            }

            if (b == NO_NUM || c == NO_NUM) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_NUM_ERROR;
                } else {
                    vm->pos += 4;
                }
                return;
            }

            vm->regs[reg] = (b % c);
            vm->pos += 4;
            break;
        case 12: // and 
            reg = vm_get_reg(vm->mem[vm->pos + 1]);
            b   = vm_get_num(vm, vm->mem[vm->pos + 2]);
            c   = vm_get_num(vm, vm->mem[vm->pos + 3]);

            if (reg == NO_REG) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_REG_ERROR;
                } else {
                    vm->pos += 4;
                }
                return;
            }

            if (b == NO_NUM || c == NO_NUM) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_NUM_ERROR;
                } else {
                    vm->pos += 4;
                }
                return;
            }

            vm->regs[reg] = (b & c) % MODULO;
            vm->pos += 4;
            break;
        case 13: // or 
            reg = vm_get_reg(vm->mem[vm->pos + 1]);
            b   = vm_get_num(vm, vm->mem[vm->pos + 2]);
            c   = vm_get_num(vm, vm->mem[vm->pos + 3]);

            if (reg == NO_REG) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_REG_ERROR;
                } else {
                    vm->pos += 4;
                }
                return;
            }

            if (b == NO_NUM || c == NO_NUM) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_NUM_ERROR;
                } else {
                    vm->pos += 4;
                }
                return;
            }

            vm->regs[reg] = (b | c) % MODULO;
            vm->pos += 4;
            break;
        case 14: // not
            reg = vm_get_reg(vm->mem[vm->pos + 1]);
            b   = vm_get_num(vm, vm->mem[vm->pos + 2]);

            if (reg == NO_REG) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_REG_ERROR;
                } else {
                    vm->pos += 3;
                }
                return;
            }

            if (b == NO_NUM) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_NUM_ERROR;
                } else {
                    vm->pos += 3;
                }
                return;
            }

            vm->regs[reg] = ((uint16_t)~b) % MODULO;
            vm->pos += 3;
            break;
        case 15: // rmem
            reg = vm_get_reg(vm->mem[vm->pos + 1]);
            b   = vm_get_num(vm, vm->mem[vm->pos + 2]);

            if (reg == NO_REG) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_REG_ERROR;
                } else {
                    vm->pos += 3;
                }
                return;
            }

            if (b == NO_NUM) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_NUM_ERROR;
                } else {
                    vm->pos += 3;
                }
                return;
            }

            vm->regs[reg] = vm->mem[b];
            vm->pos += 3;
            break;
        case 16: // wmem
            a = vm_get_num(vm, vm->mem[vm->pos + 1]);
            b = vm_get_num(vm, vm->mem[vm->pos + 2]);

            if (a == NO_NUM || b == NO_NUM) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_NUM_ERROR;
                } else {
                    vm->pos += 3;
                }
                return;
            }

            vm->mem[a] = b;
            vm->pos += 3;
            break;
        case 17: // call
            a = vm_get_num(vm, vm->mem[vm->pos + 1]);

            if (a == NO_NUM) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_NUM_ERROR;
                } else {
                    vm->pos += 2;
                }
                return;
            }

            stack_push(vm->stack, vm->pos + 2);
            if (vm->stack->status != STACK_OK) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_STACK_PUSH_FAIL_ERROR;
                } else {
                    vm->pos += 2;
                }
                return;
            }

            vm->pos = a;
            break;
        case 18: // ret
            val = stack_pop(vm->stack);
            if (vm->stack->status != STACK_OK) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_STACK_POP_FAIL_ERROR;
                } else {
                    vm->pos += 1;
                }
                return;
            }

            vm->pos = val;
            break;
        case 19: // out
            a = vm_get_num(vm, vm->mem[vm->pos + 1]);

            if (a == NO_NUM) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_NUM_ERROR;
                } else {
                    vm->pos += 2;
                }
                return;
            }

            fprintf(stdout, "%c", a);
            vm->pos += 2;
            break;
        case 20: // in
            reg = vm_get_reg(vm->mem[vm->pos + 1]);

            if (reg == NO_REG) {
                if (vm->should_skip_on_reg_or_num_err) {
                    vm->halt = true;
                    vm->status = VM_INVALID_REG_ERROR;
                } else {
                    vm->pos += 2;
                }
                return;
            }

            char ch;
            fscanf(stdin, "%1c", &ch);

            vm->regs[reg] = (uint16_t)ch;
            vm->pos += 2;
            break;
        case 21: // noop
            vm->pos += 1;
            break;
        default:
            vm->halt = true;
            vm->status = VM_INVALID_INSTRUCTION_ERROR;
            break;
    }
}
