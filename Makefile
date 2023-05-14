CC ?= cc
CFLAGS ?= -O2 -std=c99

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

atbs: serial.o modem.o main.o
	$(CC) -o $@ $^
