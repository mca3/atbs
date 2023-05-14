#include "serial.h"

#define AT_OK 0
/* Technically not, but we're using it. */
#define AT_NONE 1
#define AT_NOCARRIER 3
#define AT_ERROR 4
#define AT_BUSY 7

#define AT_REG_TERM 3

struct modem_state {
	struct serial ser;
	int dcd;

	int quiet;
	int echo;
	int verbose;

	char registers[8];
};

struct command {
	char name;
	int (*f)(struct modem_state *m, char *arg);
};

extern char *retcodes[];
extern struct command modem_commands[];

int modem_read_command(struct modem_state *m);
