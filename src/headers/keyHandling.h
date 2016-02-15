#pragma once

#define NB_ENABLE 0
#define NB_DISABLE 1

int kbhit();
void nonblock(int state);
