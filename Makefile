CC=gcc
LD=gcc

CFLAGS=-g -Wall
LDFLAGS=

SOURCES=$(shell ls src/*c)

OBJECTS=$(SOURCES:src/%.c=out/%.out)

default: $(OBJECTS)

out/%.out: src/%.c
	$(CC) $(CFLAGS) -I./src/headers/ $< -o $@
	@echo "Brawo! Kompilacja $< udana :)"

