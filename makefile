CC = gcc
CFLAGS = -Wall -Iinclude
LIBS = -linih
SRC = src/config.c src/server.c
OBJECTS = ./build/config.o ./build/server.o
TARGET = ./build/cttp


all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

./build/config.o: src/config.c
	$(CC) $(CFLAGS) $(LIBS) -c $< -o $@

./build/server.o: src/server.c
	$(CC) $(CFLAGS) $(LIBS) -c $< -o $@

debug: CFLAGS += -DDEBUG
debug: $(TARGET)

.PHONY: debug all
