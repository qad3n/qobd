#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>

#define moveCursor(row,col) printf("\033[%d;%dH",(row),(col))
#define hideCursor() printf("\033[?25l")
#define showCursor() printf("\033[?25h")
#define flushOut() fflush(stdout)

static struct termios origTerm; // original terminal settings

static inline void setRawMode(void) 
{
	struct termios raw;
	tcgetattr(STDIN_FILENO, &origTerm); // get settings

	raw = origTerm; // copy
	raw.c_lflag &= (tcflag_t)~(ICANON | ECHO); // disable canon & echo

	tcsetattr(STDIN_FILENO, TCSANOW, &raw); // apply
}

static inline void restoreTerm(void) { tcsetattr(STDIN_FILENO, TCSANOW, &origTerm); }

static inline int keyPressed(void) 
{
	struct timeval tv = { 0L, 0L }; // no wait
	fd_set fds;

	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO,&fds); // watch stdin

	return select(1, &fds, NULL, NULL, &tv) > 0; // true if ready
}

static inline char getChar(void) 
{
	char ch = 0;
	if (keyPressed()) // if input
		read(STDIN_FILENO, &ch, 1);

	return ch; // char or 0
}

#endif // TERMINAL_H
