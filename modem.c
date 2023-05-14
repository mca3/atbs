#include <ctype.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
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

static void signal_handler(int _);

static int selfpipe[2];

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

void
signal_handler(int _)
{
	write(selfpipe[1], "", 0);
}

int
cmd_dial(struct modem_state *m, char *arg)
{
	// Note that there's a good chance that most users will spend 99% of
	// their time in this command.

	if (*arg == 0)
		return AT_ERROR;

	// We don't actually care about the number, for now, but we will later.
	// For now, let's start an application.
	int pipein[2], pipeout[2], selfpipe[2];
	pid_t child;

	// Establish two pipes, one that feeds data in, and one that takes data
	// out. Another pipe for signals.
	if (pipe(pipein) == -1) {
		perror("pipe");
		return AT_ERROR;
	} else if (pipe(pipeout) == -1) {
		perror("pipe");
		return AT_ERROR;
	} else if (pipe(selfpipe) == -1) {
		perror("pipe");
		return AT_ERROR;
	}

	signal(SIGCHLD, signal_handler);

	if ((child = fork()) == 0) {
		// Child.
		// Close sides we don't own.
		close(pipein[1]); // not writing in
		close(pipeout[0]); // not reading out

		// Move file descriptors.
		dup2(pipein[0], STDIN_FILENO);
		dup2(pipeout[1], STDOUT_FILENO);

		// Ready to exec.
		execl("/bin/sh", "/bin/sh", "-c", m->command, NULL);
		perror("execl");
		exit(EXIT_FAILURE);
	}

	// Close child's sides.
	close(pipein[0]);
	close(pipeout[1]);

	char buf[32];
	int len = snprintf(buf, sizeof(buf), "CONNECT %d\r\n", m->baud);
	serial_write(&m->ser, buf, len);

	struct pollfd pfds[3] = {0};

	pfds[0].fd = m->ser.fd;
	pfds[0].events = POLLIN;
	pfds[1].fd = pipeout[0];
	pfds[1].events = POLLIN;
	pfds[2].fd = selfpipe[0];
	pfds[2].events = POLLIN;

	serial_set(&m->ser, SERIAL_DCD, 1);

	char copybuf[4096]; // TODO: good size?
	for (;;) {
		int i = poll(pfds, 3, -1);
		if (i == -1) {
			perror("poll");
			break;
		}

		// Check to see if the program is still going or not
		if (pfds[2].revents & POLLIN) {
			// No. Reap and cleanup.
			wait(NULL);
			break;
		}

		if (pfds[0].revents & POLLIN) {
			i = serial_read(&m->ser, copybuf, sizeof(copybuf));
			write(pipein[1], copybuf, i);
			continue;
		}

		if (pfds[1].revents & POLLIN) {
			i = read(pipeout[0], copybuf, sizeof(copybuf));
			serial_write(&m->ser, copybuf, i);
			continue;
		}
	}

	// Set states
	serial_set(&m->ser, SERIAL_DCD, 0);
	signal(SIGCHLD, SIG_DFL);

	// Close pipes
	close(pipein[1]);
	close(pipeout[0]);
	close(selfpipe[0]);
	close(selfpipe[1]);

	return AT_NOCARRIER;
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

		// Parsing is kinda wonky.
		if (n > 1 && isdigit(*(buf+1))) {
			for (arglen; arglen < n && arglen < 40 && isdigit(*(buf+arglen)); ++arglen)
				args[arglen-1] = *(buf+arglen);
		} else if (n > 1 && c->name == 'D') {
			// D(ial) uses everything up until EOL or a semicolon.
			for (arglen; arglen < n && arglen < 40 && *(buf+arglen) != ';'; ++arglen)
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
