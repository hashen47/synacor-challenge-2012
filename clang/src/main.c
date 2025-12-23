#include <stdio.h>
#include "../include/vm.h"

int main() {
    VM* vm = vm_init(false);
    if (vm == NULL) {
        printf("virtual machine initialization is fail\n");
        return 1;
    }

    if (vm->status != VM_OK) {
        printf("vm error: %s\n", vm_get_error_msg(vm));
        vm_free(vm);
        return 1;
    }

    vm_load_binary(vm, "../data/challenge.bin");
    if (vm->status != VM_OK) {
        printf("vm error: %s\n", vm_get_error_msg(vm));
        vm_free(vm);
        return 1;
    }

    /*
    // vm_print_memory(vm);
    vm_load_test(vm);
    if (vm->status != VM_OK) {
        printf("vm error: %s\n", vm_get_error_msg(vm));
        vm_free(vm);
        return 1;
    }
    */

    vm_process(vm);
    if (vm->status != VM_OK) {
        printf("vm error   : %s\n", vm_get_error_msg(vm));
        printf("stack error: %s\n", stack_get_error_msg(vm->stack));
        vm_free(vm);
        return 1;
    }

    vm_free(vm);
    return 0;
}
