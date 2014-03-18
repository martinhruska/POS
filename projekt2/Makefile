CC=gcc
CFLAGS=-Wall -Wextra -g -pthread -O
PROJ=proj2

all: $(PROJ)

$(PROJ): $(PROJ).c
	$(CC) $(CFLAGS) $(PROJ).c -o $@

clean:
	rm $(PROJ)
