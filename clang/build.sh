#!/bin/bash

set -x

cc="gcc"
flags="-std=c17 -ggdb -Wall -Werror -Wextra -Wswitch"
src="src"
bin="bin"

$cc $flags -o $bin/main $src/main.c $src/vm/*.c
# ./$bin/main
