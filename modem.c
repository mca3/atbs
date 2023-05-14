#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "modem.h"

static int cmd_dial(struct modem_state *m, char *arg); 	// D
static int cmd_echo(struct modem_state *m, char *arg); 	// E
static int cmd_hook(struct modem_state *m, char *arg); 	// H
static int cmd_info(struct modem_state *m, char *arg); 	// I
static int cmd_query(struct modem_state *m, char *arg);	// ?
static int cmd_quiet(struct modem_state *m, char *arg);	// Q
static int cmd_reset(struct modem_state *m, char *arg);	// Z
static int cmd_select(struct modem_state *m, char *arg);// S
static int cmd_set(struct modem_state *m, char *arg); 	// =
static int cmd_verbose(struct modem_state *m, char *arg);//V

char *retcodes[] = {
	"OK",
	"",
	"",
	"NO CARRIER",
	"ERROR",
	"",
	"BUSY",
};

struct command modem_commands[] = {
	{ .name = 'D', .f = cmd_dial },
	{ .name = 'E', .f = cmd_echo },
	{ .name = 'H', .f = cmd_hook },
	{ .name = 'I', .f = cmd_info },
	{ .name = '?', .f = cmd_query },
	{ .name = 'Q', .f = cmd_quiet },
	{ .name = 'Z', .f = cmd_reset },
	{ .name = 'S', .f = cmd_select },
	{ .name = '=', .f = cmd_set },
	{ .name = 'V', .f = cmd_verbose },
};

int
cmd_dial(struct modem_state *m, char *arg)
{
	return AT_ERROR;
}

int
cmd_echo(struct modem_state *m, char *arg)
{
	serial_set_echo(&m->ser, *arg == '1');
	return AT_OK;
}

int
cmd_hook(struct modem_state *m, char *arg)
{
	return AT_ERROR;
}

int
cmd_info(struct modem_state *m, char *arg)
{
	switch (*arg) {
	case '0': serial_write(&m->ser, "vmodem\r\n", 8); break;
	default: return AT_ERROR;
	}

	return AT_NONE;
}

int
cmd_query(struct modem_state *m, char *arg)
{
	return AT_OK;
}

int
cmd_quiet(struct modem_state *m, char *arg)
{
	m->quiet = *arg == '1';
	return AT_OK;
}

int
cmd_reset(struct modem_state *m, char *arg)
{
	m->quiet = 0;
	m->verbose = 1;
	m->echo = 0;

	m->registers[AT_REG_TERM] = '\r';
	return AT_OK;
}

int
cmd_select(struct modem_state *m, char *arg)
{
	return AT_OK;
}

int
cmd_set(struct modem_state *m, char *arg)
{
	return AT_OK;
}

int
cmd_verbose(struct modem_state *m, char *arg)
{
	m->verbose = *arg == '1';
	return AT_OK;
}

int
modem_exec_command(struct modem_state *m, char *buf, size_t n)
{
	int status = AT_OK;

	while (n > 0) {
		struct command *c = NULL;
		char args[40];
		int arglen = 1;

		// Find matching command.
		for (size_t i = 0; i < sizeof(modem_commands)/sizeof(*modem_commands); ++i) {
			if (modem_commands[i].name == *buf) {
				c = &modem_commands[i];
				break;
			}
		}

		if (!c)
			return AT_ERROR;

		if (n > 1 && isdigit(*(buf+1))) {
			for (arglen; arglen < n && arglen < 40 && isdigit(*(buf+arglen)); ++arglen)
				args[arglen-1] = *(buf+arglen);
		} else {
			args[0] = '0';
		}

		args[arglen] = 0;
		fprintf(stderr, "Execute: %c arg %s\n", c->name, args);

		if ((status = c->f(m, args)) != AT_OK)
			return status;

		buf += arglen;
		n -= arglen;
	}

	return status;
}

int
modem_read_command(struct modem_state *m)
{
	char buf[40] = {0};
	size_t n = 0;
	char c;
	int s;

	// HACK
	while (serial_read(&m->ser, &c, 1) == 1) {
		if (c == m->registers[AT_REG_TERM] || c == 0)
			break;

		buf[n++] = c;
		if (n == 40)
			break;
	}

	if (n < 2 || buf[0] != 'A' || buf[1] != 'T')
		return 0;

	serial_write(&m->ser, "\r\n", 2);

	s = modem_exec_command(m, buf+2, n-2);

	if (m->quiet || s == AT_NONE)
		return s;

	if (m->verbose) {
		serial_write(&m->ser, retcodes[s], strlen(retcodes[s]));
		serial_write(&m->ser, "\r\n", 2);
	} else {
		char buf[2] = "0\r";
		buf[0] = '0' + s;
		serial_write(&m->ser, buf, 2);
	}

	serial_flush(&m->ser);

	return s;
}
