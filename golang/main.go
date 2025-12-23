package main

import (
	"io"
	"os"
	"fmt"
	"vm/vm"
)

func handleErr(err error) {
	if err != nil {
		fmt.Printf("Err: %s\n", err.Error())
		os.Exit(1)
	}
}

func loadBin() *[]byte {
	path := "../data/challenge.bin"

	file, err := os.Open(path)
	handleErr(err)
	defer file.Close()

	bin, err := io.ReadAll(file)
	handleErr(err)

	return &bin
}

func main() {
	machine := vm.NewVM()
	machine.LoadBinary(loadBin())
	// machine.LoadTest()
	machine.Process()
}
