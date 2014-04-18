CC=gcc
CFLAGS=-Wall -Wextra -ansi -pthread
PROJ=proj03

all: $(PROJ)

$(PROJ): $(PROJ).c
	$(CC) $(CFLAGS) $(PROJ).c -o $@

clean:
	rm $(PROJ)
