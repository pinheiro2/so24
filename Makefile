CC = gcc
CFLAGS = -Wall -g -Iinclude
LDFLAGS =

all: folders orchestrator client

orchestrator: bin/orchestrator

client: bin/client

folders:
	@mkdir -p src include obj bin tmp

bin/orchestrator: obj/orchestrator.o obj/mysystem.o obj/binary-heap.o obj/execPipeline.o
	$(CC) $(LDFLAGS) $^ -o $@

bin/client: obj/client.o
	$(CC) $(LDFLAGS) $^ -o $@

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f obj/* tmp/* bin/*
