CC=gcc
CFLAGS=-Wall -Wextra -g -pthread -O
PROJ=proj3

all: $(PROJ)

$(PROJ): $(PROJ).c
	$(CC) $(CFLAGS) $(PROJ).c -o $@

clean:
	rm $(PROJ)
