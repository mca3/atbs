# atbs

atbs (AT Bootstrap) is a tool to bootstrap PPP connections (or whatever else)
over serial, useful for connecting ancient machines to the Internet like it's
the late 1990s.

atbs emulates just enough of a Hayes-compatible modem to dial a number and
establish a connection to, say, a PPP server.
atbs has been successfully used to do just that on a physical Windows XP
machine.

## Usage

Download and compile, there are no dependencies.
It is known to work on Linux.

atbs takes arguments that control how it works and what settings it uses.
You will likely be interested in the following:

- `-b`: baud rate (default: 9600)
- `-c`: command (default: `ppp notty`)
- `-t`: serial device (default: `/dev/tty`, current tty)

Once started and connected to a computer over serial, you can connect to the
modem and dial any number to execute the command you specified.

## Known issues

- Incomplete modem emulation

As atbs is in a very early stage in its development, just enough has been
written to connect to the Internet.
Modem escapes are not supported, and as such it is impossible to disconnect.
Many, many commands are not supported.

Additionally, control lines have largely been left unimplemented, and it is
unknown if the little code written for them even works properly.

- Little instructions

I'll write a guide on how to get a PPP server up and running with this
eventually, but for now you're left on your own.

- Largely untested

The only testing I have done with this has me plugging it into a Windows XP
machine and hoping for the best; while it hasn't let me down yet, it will most
certainly let you down if you aren't using it exactly like how I am.
