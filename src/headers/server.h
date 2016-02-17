#pragma once

#include "struct.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define KEYP1 31415926 
#define KEYP2 11111111 
#define KEYMEM 96

typedef struct Price {
	int light;
	int heavy;
	int cavalry;
	int workers;
} Price;

typedef struct AttackForce {
	float light;
	float heavy;
	float cavalry;
} AttackForce;

typedef struct DefenceForce {
	float light;
	float heavy;
	float cavalry;
} DefenceForce;

typedef struct State {
	int light[2];
	int heavy[2];
	int cavalry[2];
	int workers[2];
	int points[2];
	int resources[2];
	char end;
} State;

typedef struct QueueId {
	int initialQ;
	int player1Q;
	int player2Q;
} QueueId;

QueueId* queueIdList;
State* state;

Price setPrices();
AttackForce setAttackForce();
DefenceForce setDefenceForce();
void initStateMemory();
void sendGameState(State* state, int player);
void f();
void initQueues();
void initConnection();
void waitingForPlayers();
void initData(State* state);
void sendResources(State* state);
void receiveBuild(Price prices, State* state);
void printBuild(Build build, int player);
void receiveAttack(AttackForce attackForce, DefenceForce defenceForce, State* state);
void destruction(State* state);
