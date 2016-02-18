#pragma once

#include "struct.h"
#include "keyHandling.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

char status[111];
char infoLog[4][111];
int beatSkips;

void clear();
void printGameState(Data data);
void updateLog(Data data);
void printLog();
void printMenu();
void sendBuildMessage(Build build, int id2);
void building(Data data, int id2);
void sendAttackMessage(Attack attack, int id2);
void attacking(Data data, int id2);
void update(Data data);
void heartbeat(int id2);
