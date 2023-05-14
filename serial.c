/* for CRTSCTS */
#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "serial.h"

// Sets raw mode at baud.
static int
set_raw(struct serial *s, speed_t baud)
{
	// Save old termios
	if (tcgetattr(s->fd, &s->t_old) == -1)
		return -1;

	// Set raw mode, and other flags
	s->t_current.c_cflag = CS8 | CLOCAL | CREAD | CRTSCTS;
	s->t_current.c_iflag = (IGNBRK);
	s->t_current.c_oflag = 0;
	s->t_current.c_lflag = 0;
	s->t_current.c_cc[VMIN]=1;
	s->t_current.c_cc[VTIME]=0;
	cfsetispeed(&s->t_current, baud);
	cfsetospeed(&s->t_current, baud);

	return tcsetattr(s->fd, TCSANOW, &s->t_current);
}

/* Opens a file as the serial object.
 * The serial struct will be cleared. */
int
serial_new(struct serial *s, char *path, speed_t baud)
{
	memset(s, 0, sizeof(*s));
	s->fd = open(path, O_RDWR | O_NOCTTY);

	if (s->fd == -1)
		return -1;
	if (!isatty(s->fd)) {
		close(s->fd);
		errno = ENOTTY;
		return -1;
	}

	if (set_raw(s, baud) == -1) {
		close(s->fd);
		return -1;
	}

	return 0;
}

/* Closes the serial connection and restores termios. */
int
serial_close(struct serial *s)
{
	tcsetattr(s->fd, TCSANOW, &s->t_old);
	close(s->fd);
	return 0;
}

/* Read data from the serial fd. */
int
serial_read(struct serial *s, void *buf, size_t n)
{
	return read(s->fd, buf, n);
}

/* Write data to the serial fd. */
int
serial_write(struct serial *s, void *buf, size_t n)
{
	return write(s->fd, buf, n);
}

/* Write outstanding data. */
int
serial_flush(struct serial *s)
{
	return tcdrain(s->fd);
}

/* Set control lines. */
int
serial_set(struct serial *s, int flags, int val)
{
	int mask;
	int ctrl;

	if (flags & SERIAL_DCD)
		mask |= TIOCM_DTR;

	// Get control lines
	if (ioctl(s->fd, TIOCMGET, &ctrl) == -1)
		return -1;
	
	if (val)
		ctrl &= ~mask;
	else
		ctrl |= mask;

	return ioctl(s->fd, TIOCMSET, &ctrl);
}

/* Controls echo. */
int
serial_set_echo(struct serial *s, int val)
{
	s->t_current.c_lflag = val ? ECHO : 0;
	return tcsetattr(s->fd, TCSANOW, &s->t_current);
}
