#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "serial.h"
#include "modem.h"

extern char *optarg;
extern int optind, opterr, optopt;

void
usage(char *argv0)
{
	fprintf(stderr, "usage: %s [-b baud] [-t /dev/tty]\n", argv0);
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	char *tty = "/dev/tty";
	int opt;
	int baud = 9600;
	speed_t actual_baud = B9600;
	struct modem_state m = {0};

	while ((opt = getopt(argc, argv, "b:t:")) != -1) {
		switch (opt) {
		case 'b': baud = atoi(optarg); break;
		case 't': tty = optarg; break;
		default: usage(argv[0]);
		}
	}

	switch (baud) {
	case 9600: actual_baud = B9600; break;
	default: fprintf(stderr, "invalid baud: %d\n", baud); exit(EXIT_FAILURE);
	}

	m.echo = 1;
	m.verbose = 1;
	m.registers[AT_REG_TERM] = '\r';

	serial_new(&m.ser, tty, baud);
	serial_set_echo(&m.ser, 1);

	for (;;)
		modem_read_command(&m);

	serial_close(&m.ser);
}
