package vm

import (
	"errors"
)

var ErrStackEmpty = errors.New("stack is empty")

type Stack struct {
	data []uint16
}

func NewStack() *Stack {
	s := Stack{}
	s.data = make([]uint16, 0)
	return &s
}

func (s *Stack) Push(val uint16) {
	s.data = append(s.data, val)
}

func (s *Stack) Pop() (uint16, error) {
	if len(s.data) == 0 {
		return uint16(0), ErrStackEmpty
	}
	val := s.data[len(s.data)-1]
	s.data = s.data[0:len(s.data)-1]
	return val, nil
}
