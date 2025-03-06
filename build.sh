#!/bin/bash

CC="gcc"
CFLAGS="-Wall -Iinclude" 
LIBS="-linih"
SRC="src/config.c src/server.c" 
TARGET="./build/server"

all() {
	echo "Compiling"
	$CC $CFLAGS $1 -o $TARGET $SRC $LIBS
	if [[ $? == 0 ]]; then
		echo "Compilation Successful"
	else
		echo "Compilation Failed"
	fi
}

clean() {
	rm -rf $TARGET
}

usage() {
    echo "Usage: $0 {all|run|clean}"
    echo "  all   : Compile the program"
    echo "  run   : Compile and run the program"
    echo "  clean : Remove the compiled program"
    exit 1
}

if [[ $# == 0 ]]; then
	usage
fi

if [[ $1 == "all" ]]; then
	all
fi

if [[ $1 == "run" ]]; then
	all
	echo "Running $TARGET"
	./$TARGET
	clean
fi

if [[ $1 == "debug" ]]; then
	all -DDEBUG
	echo "Running $TARGET"
	./$TARGET
	clean
fi

if [[ $1 == "clean" ]]; then
	clean
fi

