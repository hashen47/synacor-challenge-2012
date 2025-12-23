package vm

import (
	"os"
	"fmt"
	"errors"
	"encoding/binary"
)

const (
	MODULO uint16 = 32768
)

type VM struct {
	mem   [32768]uint16
	stack *Stack 
	regs  [8]uint16
	pos   uint16
	halt  bool
}

func NewVM() *VM {
	vm := VM{}
	vm.stack = NewStack()
	vm.ResetMemory()
	return &vm
}

func (vm *VM) ResetMemory() {
	// NOTE: fill memory with noop instruction 
	for i := 0; i < len(vm.mem); i++ {
		vm.mem[i] = uint16(21) // noop
	}
}

func (vm *VM) LoadBinary(bin *[]byte) {
	for i := 0; i < len(*bin)-1; i += 2 {
		n := binary.LittleEndian.Uint16((*bin)[i:i+2])
		vm.mem[i/2] = n
	}
}

func (vm *VM) LoadTest() {
	// NOTE: test output should be 'E' with a newline
	vm.ResetMemory()
	vm.SetMemory([]uint16{9,32768,32769,4,19,32768,19,10,0})
	vm.regs[1] = 'A'
}

func (vm *VM) SetMemory(mem []uint16) {
	for i := 0; i < len(mem); i++ {
		vm.mem[i] = mem[i]
	}
}

func (vm *VM) PrintMemory() {
	for i := 0; i < len(vm.mem); i++ {
		fmt.Printf("i: (%5d), val: (%5d)\n", i, vm.mem[i])
	}
}

func (vm *VM) Process() {
	for !vm.halt && int(vm.pos) < len(vm.mem) && vm.pos >= 0 {
		err := vm.next()
		if err != nil {
			vm.halt = true
			fmt.Printf("Err: %v\n", err.Error())
		}
	}
}

func (vm *VM) getReg(a uint16) int {
	if a < 32768 || a > 32775 {
		return -1
	}
	return int(a % 32768)
}

func (vm *VM) getNum(n uint16) (uint16, bool) {
	/*
	if n > 32767 {
		return uint16(0), false
	}
	return n, true 
	*/
	if n > 32767 {
		reg := vm.getReg(n)
		if reg == -1 {
			return uint16(0), false
		}
		return vm.regs[reg], true
	}
	return n, true 
}

func (vm *VM) next() error {
	switch vm.mem[vm.pos] {
	case 0: // halt
		vm.halt = true
		vm.pos += 1
		fmt.Println("halt is happend")
	case 1: // set
		reg   := vm.getReg(vm.mem[vm.pos + 1])
		b, ok := vm.getNum(vm.mem[vm.pos + 2])

		if reg == -1 || !ok {
			vm.pos += 3
			return nil
		}

		vm.regs[reg] = b
		vm.pos += 3
	case 2: // push 
		a, ok := vm.getNum(vm.mem[vm.pos + 1])

		if !ok {
			vm.pos += 2
			return nil
		}

		vm.stack.Push(a)
		vm.pos += 2
	case 3: // pop
		a := vm.getReg(vm.mem[vm.pos + 1])

		if a == -1 {
			vm.pos += 2
			return nil
		}

		val, err := vm.stack.Pop()
		if err != nil {
			vm.pos += 2
			return err
		}

		vm.regs[a] = val
		vm.pos += 2
	case 4: // eq
		reg    := vm.getReg(vm.mem[vm.pos + 1]) 
		b, ok1 := vm.getNum(vm.mem[vm.pos + 2])
		c, ok2 := vm.getNum(vm.mem[vm.pos + 3])
		
		if reg == -1 || !ok1 || !ok2 {
			vm.pos += 4
			return nil
		}

		vm.regs[reg] = uint16(0)
		if b == c {
			vm.regs[reg] = uint16(1)
		}
		vm.pos += 4
	case 5: // gt
		reg    := vm.getReg(vm.mem[vm.pos + 1]) 
		b, ok1 := vm.getNum(vm.mem[vm.pos + 2])
		c, ok2 := vm.getNum(vm.mem[vm.pos + 3])
		
		if reg == -1 || !ok1 || !ok2 {
			vm.pos += 4
			return nil
		}

		vm.regs[reg] = uint16(0)
		if b > c {
			vm.regs[reg] = uint16(1)
		}
		vm.pos += 4
	case 6: // jmp
		a, ok := vm.getNum(vm.mem[vm.pos + 1])

		if !ok {
			vm.pos += 2
			return nil
		}

		vm.pos = a
	case 7: // jt
		a, ok1 := vm.getNum(vm.mem[vm.pos + 1])
		b, ok2 := vm.getNum(vm.mem[vm.pos + 2])

		if !ok1 || !ok2 {
			vm.pos += 3
			return nil
		}

		if a == 0 {
			vm.pos += 3
			return nil
		}

		vm.pos = b
	case 8: // jf
		a, ok1 := vm.getNum(vm.mem[vm.pos + 1])
		b, ok2 := vm.getNum(vm.mem[vm.pos + 2])

		if !ok1 || !ok2 {
			vm.pos += 3
			return nil
		}

		if a != 0 {
			vm.pos += 3
			return nil
		}

		vm.pos = b
	case 9: // add
		reg    := vm.getReg(vm.mem[vm.pos + 1])
		b, ok1 := vm.getNum(vm.mem[vm.pos + 2])
		c, ok2 := vm.getNum(vm.mem[vm.pos + 3])

		if reg == -1 || !ok1 || !ok2 {
			vm.pos += 4
			return nil
		}

		vm.regs[reg] = (b + c) % MODULO
		vm.pos += 4
	case 10: // mult
		reg    := vm.getReg(vm.mem[vm.pos + 1])
		b, ok1 := vm.getNum(vm.mem[vm.pos + 2])
		c, ok2 := vm.getNum(vm.mem[vm.pos + 3])

		if reg == -1 || !ok1 || !ok2 {
			vm.pos += 4
			return nil
		}

		vm.regs[reg] = (b * c) % MODULO
		vm.pos += 4
	case 11: // mod
		reg    := vm.getReg(vm.mem[vm.pos + 1])
		b, ok1 := vm.getNum(vm.mem[vm.pos + 2])
		c, ok2 := vm.getNum(vm.mem[vm.pos + 3])

		if reg == -1 || !ok1 || !ok2 {
			vm.pos += 4
			return nil
		}

		vm.regs[reg] = b % c
		vm.pos += 4
	case 12: // and
		reg    := vm.getReg(vm.mem[vm.pos + 1])
		b, ok1 := vm.getNum(vm.mem[vm.pos + 2])
		c, ok2 := vm.getNum(vm.mem[vm.pos + 3])

		if reg == -1 || !ok1 || !ok2 {
			vm.pos += 4
			return nil
		}

		vm.regs[reg] = (b & c) % MODULO
		vm.pos += 4
	case 13: // or
		reg    := vm.getReg(vm.mem[vm.pos + 1])
		b, ok1 := vm.getNum(vm.mem[vm.pos + 2])
		c, ok2 := vm.getNum(vm.mem[vm.pos + 3])

		if reg == -1 || !ok1 || !ok2 {
			vm.pos += 4
			return nil
		}

		vm.regs[reg] = (b | c) % MODULO
		vm.pos += 4
	case 14: // not 
		reg   := vm.getReg(vm.mem[vm.pos + 1])
		b, ok := vm.getNum(vm.mem[vm.pos + 2])

		if reg == -1 || !ok {
			vm.pos += 3
			return nil
		}

		vm.regs[reg] = (^b) % MODULO
		vm.pos += 3
	case 15: // rmem
		reg   := vm.getReg(vm.mem[vm.pos + 1])
		b, ok := vm.getNum(vm.mem[vm.pos + 2])

		if reg == -1 || !ok {
			vm.pos += 3
			return nil
		}

		vm.regs[reg] = vm.mem[b]
		vm.pos += 3
	case 16: // wmem
		a, ok1 := vm.getNum(vm.mem[vm.pos + 1])
		b, ok2 := vm.getNum(vm.mem[vm.pos + 2])

		if !ok1 || !ok2 {
			vm.pos += 3
			return nil
		}

		vm.mem[a] = b
		vm.pos += 3
	case 17: // call
		a, ok := vm.getNum(vm.mem[vm.pos + 1])
		if !ok {
			vm.pos += 2
			return nil
		}

		vm.stack.Push(uint16(vm.pos + 2))
		vm.pos = a
	case 18: // ret
		val, err := vm.stack.Pop()
		if err != nil {
			vm.pos += 1
			return err
		}

		vm.pos = val
	case 19: // out
		a, ok := vm.getNum(vm.mem[vm.pos + 1])

		if !ok {
			vm.pos += 2
			return nil
		}

		fmt.Printf("%c", a)
		vm.pos += 2
	case 20: // in
		reg := vm.getReg(vm.mem[vm.pos + 1])

		if reg == -1 {
			vm.pos += 2
			return nil
		}

		var ch rune
		fmt.Fscanf(os.Stdin, "%c", &ch)
		vm.regs[reg] = uint16(ch)
		vm.pos += 2
	case 21: // noop
		vm.pos += 1
	default:
		return errors.New("Invalid Instruction")
	}

	return nil
}

