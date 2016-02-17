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

typedef struct Price {
	int light;
	int heavy;
	int cavalry;
	int workers;
} Price;

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
void initStateMemory();
void sendGameState(State* state, int player);
void f();
void initQueues();
void initConnection();
void waitingForPlayers();
void initData(State* state);
void receiveBuild(Price prices, State* state);
void printBuild(Build build, int player);
void destruction(State* state);

int main() {
	
	Price prices = setPrices();

	initStateMemory();
	initQueues();
	signal(SIGINT, f);
	initConnection();
	waitingForPlayers();
	initData(state);

	while(1) {
		sleep(1);
		int player;
		for(player = 0; player < 2; player++) {
			state->resources[player] += 50 + state->workers[player]*5;
			sendGameState(state, player);
		}

		receiveBuild(prices, state);
	}
}

Price setPrices() {
	Price price;
	price.light = 100;
	price.heavy = 250;
	price.cavalry = 550;
	price.workers = 150;
	return price;
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
	if(!player) data.mtype = 1;
	else data.mtype = 1;
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
//	else printf("Update sent to player #%d = %d\n", player+1, state->resources[player]);
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
	int del;

	/* Opening/creating first queue */
	key_t key = KEY;
	int id = msgget(key, 0640);
	if(id == -1) {
		id = msgget(key, IPC_CREAT | 0640);
		if(id == -1) perror("msgget error");
	}
	else {
		printf("Another server already working\n");
		exit(0);
//		del = msgctl(id, IPC_RMID, 0);
//		if(del == -1) perror("msgctl error");
//		id = msgget(key, IPC_CREAT | 0640);
	}

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
	/* Sending private queues' keys */
	Init init;
	int players;
	for(players = 0; players < 2; players++) {
		init.mtype = 1;
		if(players == 0) init.nextMsg = KEYP1;
		else if(players == 1) init.nextMsg = KEYP2;
		int i = msgsnd(queueIdList->initialQ, &init, sizeof(init.nextMsg), IPC_NOWAIT);
		if(i == -1) perror("msgsnd error");
		else printf("Init #%d sent. Key = %d\n", players+1, init.nextMsg);
	}
}

void waitingForPlayers() {
	int players = 0;
	while(players < 2) {
		Init init;
		long type = 2;
		int i = msgrcv(queueIdList->initialQ, &init, sizeof(init.nextMsg), type, 0);
		if(i == -1) {
			printf("Queue error\n");
			exit(0);
		}
		else players++;
	}
}

void initData(State* state) {
	/* Sending 300 entities of resources to players */
	int player;
	for(player = 0; player < 2; player++) {
		state->light[player] = 0;
		state->heavy[player] = 0;
		state->cavalry[player] = 0;
		state->workers[player] = 0;
		state->points[player] = 0;
		state->resources[player] = 300;
		state->end = 'n';
		
		sendGameState(state, player);
	}
}

void receiveBuild(Price prices, State* state) {
	long type = 2;
	int player;

	int p1 = fork();
	if(p1 == 0) player = 0;
	else {
		int p2 = fork();
		if(p2 == 0) player = 1;
		else return;
	}

	Build build;
	int i;
	if(!player) i = msgrcv(queueIdList->player1Q, &build, sizeof(Build) - sizeof(build.mtype), type, IPC_NOWAIT);
	else i = msgrcv(queueIdList->player2Q, &build, sizeof(Build) - sizeof(build.mtype), type, IPC_NOWAIT);
	if(i != -1) {
		if(build.light) {
			if(build.light*prices.light <= state->resources[player]) {
				printBuild(build, player);
				int i;
				for(i = build.light; i > 0; i--) {
					sleep(2);
					state->light[player]++;
					sendGameState(state, player);
					printf("Light warrior has been recruited\n");
				}
			}
			else { printf("Player #%d, not enough resources\n", player+1); }
		}
		else if(build.heavy) {
			if(build.heavy*prices.heavy <= state->resources[player]) {
				printBuild(build, player);
				int i;
				for(i = build.heavy; i > 0; i--) {
					sleep(3);
					state->heavy[player]++;
					sendGameState(state, player);
					printf("Heavy warrior has been recruited\n");
				}
			}
			else { printf("Player #%d, not enough resources\n", player+1); }
		}
		else if(build.cavalry) {
			if(build.cavalry*prices.cavalry <= state->resources[player]) {
				printBuild(build, player);
				int i;
				for(i = build.cavalry; i > 0; i--) {
					sleep(5);
					state->cavalry[player]++;
					sendGameState(state, player);
					printf("Cavalryman has been recruited\n");
				}
			}
			else { printf("Player #%d, not enough resources\n", player+1); }
		}
		else if(build.workers) {
			if(build.workers*prices.workers <= state->resources[player]) {
				printBuild(build, player);
				int i;
				for(i = build.workers; i > 0; i--) {
					sleep(2);
					state->workers[player]++;
					sendGameState(state, player);
					printf("Worker has been recruited\n");
				}
			}
			else { printf("Player #%d, not enough resources\n", player+1); }
		}
	}
	exit(0);	
}

void printBuild(Build build, int player) {
	printf("Player #%d: %d light, %d heavy, %d cavalry, %d workers\n", 
				player+1, build.light, build.heavy, build.cavalry, build.workers);
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

