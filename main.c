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
	fprintf(stderr, "usage: %s [-b baud] [-t /dev/tty] [-c command]\n", argv0);
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
	m.command = "pppd notty";

	while ((opt = getopt(argc, argv, "b:c:t:")) != -1) {
		switch (opt) {
		case 'b': baud = m.baud = atoi(optarg); break;
		case 'c': m.command = optarg; break;
		case 't': tty = optarg; break;
		default: usage(argv[0]);
		}
	}

	switch (baud) {
	case 50: actual_baud = B50; break;
	case 75: actual_baud = B75; break;
	case 110: actual_baud = B110; break;
	case 134: actual_baud = B134; break;
	case 150: actual_baud = B150; break;
	case 200: actual_baud = B200; break;
	case 300: actual_baud = B300; break;
	case 600: actual_baud = B600; break;
	case 1200: actual_baud = B1200; break;
	case 1800: actual_baud = B1800; break;
	case 2400: actual_baud = B2400; break;
	case 4800: actual_baud = B4800; break;
	case 9600: actual_baud = B9600; break;
	case 19200: actual_baud = B19200; break;
	case 38400: actual_baud = B38400; break;
	case 57600: actual_baud = B57600; break;
	case 115200: actual_baud = B115200; break;
	case 230400: actual_baud = B230400; break;
	case 460800: actual_baud = B460800; break;
	case 500000: actual_baud = B500000; break;
	case 576000: actual_baud = B576000; break;
	case 921600: actual_baud = B921600; break;
	case 1000000: actual_baud = B1000000; break;
	case 1152000: actual_baud = B1152000; break;
	case 1500000: actual_baud = B1500000; break;
	case 2000000: actual_baud = B2000000; break;
	default: fprintf(stderr, "invalid baud rate: %d\n", baud); exit(EXIT_FAILURE);
	}

	m.echo = 1;
	m.verbose = 1;
	m.registers[AT_REG_TERM] = '\r';
	m.baud = baud;

	serial_new(&m.ser, tty, actual_baud);
	serial_set_echo(&m.ser, 1);

	for (;;)
		modem_read_command(&m);

	serial_close(&m.ser);
}
