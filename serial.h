#ifndef SERIAL_H
#define SERIAL_H
#include <stddef.h>
#include <termios.h>

// For serial_set
#define SERIAL_DCD (1 << 0)

struct serial {
	int fd;
	struct termios t_old, t_current;
};

int serial_new(struct serial *s, char *path, speed_t baud);
int serial_close(struct serial *s);

int serial_read(struct serial *s, void *buf, size_t n);
int serial_write(struct serial *s, void *buf, size_t n);
int serial_flush(struct serial *s);
int serial_set(struct serial *s, int flags, int val);
int serial_set_echo(struct serial *s, int val);
#endif
