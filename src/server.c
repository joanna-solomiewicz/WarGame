#include "headers/struct.h"

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

void initStateMemory();
void sendGameState(State* state, int player);
void f();
void initQueues();
void initConnection();
void initData(State* state);
void destruction(State* state);

int main() {
	
	initStateMemory();
	initQueues();
	signal(SIGINT, f);
	initConnection();
	initData(state);

	while(1) {
		sleep(1);
		state->resources[0] += 50 + state->workers[0]*5;
		sendGameState(state, 0);

		state->resources[1] += 50 + state->workers[1]*5;
		sendGameState(state, 1);
	}
}

void initStateMemory() {
	/* Creating type State* shared memory */
	key_t key = KEYMEM;
	int shmid = shmget(key, sizeof(State), IPC_CREAT | 0640);
	if(shmid == -1) perror("shmget error");
	state = (State*)shmat(shmid, NULL, 0);
}

void sendGameState(State* state, int player) {
	Data data;
	if(!player) data.mtype = KEYP1;
	else data.mtype = KEYP2;
	data.light = state->light[player];
	data.heavy = state->heavy[player];
	data.cavalry = state->cavalry[player];
	data.workers = state->workers[player];
	data.points = state->points[player];
	data.resources = state->resources[player];
	strcpy(data.info, "Game State\0");
	data.end = state->end;
	
	int i;
	if(!player) i = msgsnd(queueIdList->player1Q, &data, sizeof(Data) - sizeof(data.mtype), IPC_NOWAIT);
	else i = msgsnd(queueIdList->player2Q, &data, sizeof(Data) - sizeof(data.mtype), IPC_NOWAIT);
	if(i == -1) perror("msgsnd error");
	else printf("Update sent to player #%d = %d\n", player+1, state->resources[player]);
}

void f() {
	printf("Signal handling\n");

	Init init;
	init.mtype = 4;
	init.nextMsg = KEYP1;
	int i = msgsnd(queueIdList->initialQ, &init, sizeof(init.nextMsg), IPC_NOWAIT);
	if(i == -1) perror("msgsnd error");
	else {
		printf("I've been killed !\n");
		destruction(state);
	}

	exit(0);
}

void initQueues() {
	/* Opening/creating first queue */
	key_t key = KEY;
	int id = msgget(key, IPC_CREAT | 0640);
	if(id == -1) perror("msgget error");

	int del;

	/* Creating private queue for player #1 */
	key = KEYP1;
	int idP1 = msgget(key, 0640);
	if(idP1 == -1) {
		idP1 = msgget(key, IPC_CREAT | 0640);
		if(idP1 == -1) perror("msgget error");
	}
	else {
		del = msgctl(idP1, IPC_RMID, 0);
		if(del == -1) perror("msgctl error");
		idP1 = msgget(key, IPC_CREAT | 0640);
	}

	/* Creating private queue for player #2 */
	key = KEYP2;
	int idP2 = msgget(key, 0640);
	if(idP2 == -1) {
		idP2 = msgget(key, IPC_CREAT | 0640);
		if(idP2 == -1) perror("msgget error");
	}
	else {
		del = msgctl(idP2, IPC_RMID, 0);
		if(del == -1) perror("msgctl error");
		idP2 = msgget(key, IPC_CREAT | 0640);
	}

	/* Creating type QueueId* shared memory */
	key = 69;
	int shmid = shmget(key, sizeof(State), IPC_CREAT | 0640);
	if(shmid == -1) perror("shmget error");
	queueIdList = (QueueId*)shmat(shmid, NULL, 0);
	queueIdList->initialQ = id;
	queueIdList->player1Q = idP1;
	queueIdList->player2Q = idP2;
}

void initConnection() {
	/* Sending private queues' keys to players*/
	Init init;
	int i;
	int players = 0;
	int initNr = 1;
	int doBreak = 1;
	while(doBreak) {
		long type = 1;
		i = msgrcv(queueIdList->initialQ, &init, sizeof(init.nextMsg), type, 0);
		if(i == -1) perror("msgrcv error");
		else { 
			printf("Init received %d\n", init.nextMsg); 
			players++; 

			init.mtype = 2;
			if(players == 1) init.nextMsg = KEYP1;
			else if(players == 2) init.nextMsg = KEYP2;

			i = msgsnd(queueIdList->initialQ, &init, sizeof(init.nextMsg), IPC_NOWAIT);
			if(i == -1) perror("msgsnd error");
			else printf("Init #%d sent back. Key = %d\n", initNr, init.nextMsg);

			initNr++;
		}
		if(players == 2) doBreak = 0;
	}
}

void initData(State* state) {
	/* Sending 300 entities of resources to players */
	state->resources[0] = 300;
	sendGameState(state, 0);

	state->resources[1] = 300;
	sendGameState(state, 1);
}

void destruction(State* state) {
	/* Destruction */
	int destructor = msgctl(queueIdList->initialQ, IPC_RMID, 0);
	if(destructor == -1) perror("destructor error");
	else printf("Queue #1 key = %d destructed\n", KEY);

	destructor = msgctl(queueIdList->player1Q, IPC_RMID, 0);
	if(destructor == -1) perror("destructor error");
	else printf("Queue #2 key = %d destructed\n", KEYP1);

	destructor = msgctl(queueIdList->player2Q, IPC_RMID, 0);
	if(destructor == -1) perror("destructor error");
	else printf("Queue #3 key = %d destructed\n", KEYP2);

	shmdt(state);
}

