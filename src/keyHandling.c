#include <sys/select.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <stdio.h>

#include "keyHandling.h"

#define NB_ENABLE 0
#define NB_DISABLE 1

int kbhit() {
	struct timeval tv;
	fd_set fds;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);
	select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
	return FD_ISSET(STDIN_FILENO, &fds);
}

void nonblock(int state) {
	struct termios ttystate;
	  
	tcgetattr(STDIN_FILENO, &ttystate);
				  
	if (state == NB_ENABLE) {
		ttystate.c_lflag &= ~ICANON & ~ECHO;
		ttystate.c_cc[VMIN] = 1;
	}
	else if (state == NB_DISABLE) {
		ttystate.c_lflag |= ICANON;
	}
	tcsetattr(STDIN_FILENO, TCSANOW, (const struct termios*)&ttystate);
}
