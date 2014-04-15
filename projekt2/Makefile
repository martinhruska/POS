CC=gcc
CFLAGS=-Wall -Wextra -g -lpthread -O
PROJ=proj02

all: $(PROJ)

$(PROJ): $(PROJ).c
	$(CC) $(CFLAGS) $(PROJ).c -o $@

clean:
	rm $(PROJ)
