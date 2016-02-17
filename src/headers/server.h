#pragma once

#include "struct.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define KEYP1 31415926 
#define KEYP2 11111111 
#define KEYMEM 96
#define KEYSEM 13

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

Price prices;
AttackForce attackForce;
DefenceForce defenceForce;
State* state;
struct sembuf sem;
int semaphoreID;
QueueId* queueIdList;

Price setPrices();
AttackForce setAttackForce();
DefenceForce setDefenceForce();
void initConsts();
void initStateMemory();
void initSemaphore();
void P(int semid, struct sembuf sem);
void V(int semid, struct sembuf sem);
void clear();
void printGameState();
void sendGameState(int player, char info[]);
void f();
void initQueues();
void initConnection();
void waitingForPlayers();
void initData();
void sendResources();
void receiveBuild();
void printBuild(Build build, int player);
void receiveAttack();
void destruction();

