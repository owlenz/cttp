#!/bin/sh

killall server

if [[ ! -d "build" ]];then
	mkdir build
fi

gcc -o build/server src/server.c -I./include/
gcc -o build/client src/client.c -I./include/

./build/server &

# ./build/client $1
