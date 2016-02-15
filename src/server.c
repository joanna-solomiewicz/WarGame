#include "struct.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

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

Data sendGameState(State* state, int player) {
	Data data;
	if(!player) data.mtype = KEYP1;
	else data.mtype = KEYP2;
	data.light = state->light[player];
	data.heavy = state->heavy[player];
	data.cavalry = state->cavalry[player];
	data.workers = state->workers[player];
	data.points = state->points[player];
	data.resources = state->resources[player];
//	strcpy(data.info, "Game State\0");
	data.end = state->end;
	
	return data;
}

int main() {
	
	State* state;
	key_t key = KEYMEM;
	int shmid = shmget(key, sizeof(State), IPC_CREAT | 0640);
	if(shmid == -1) perror("shmget error");
	state = (State*)shmat(shmid, NULL, 0);

	key = KEY;
	int id = msgget(key, IPC_CREAT | 0640);
	if(id == -1) perror("msgget error");

	key = KEYP1;
	int idP1 = msgget(key, IPC_CREAT | 0640);
	if(idP1 == -1) perror("msgget error");

	key = KEYP2;
	int idP2 = msgget(key, IPC_CREAT | 0640);
	if(idP2 == -1) perror("msgget error");

	Init init;
	int i;
	int players = 0;
	int initNr = 1;
	
	int doBreak = 1;
	while(doBreak) {
		long type = 1;
		i = msgrcv(id, &init, sizeof(init.nextMsg), type, 0);
		if(i == -1) perror("msgrcv error");
		else { 
			printf("Init received %d\n", init.nextMsg); 
			players++; 

			init.mtype = 2;
			if(players == 1) init.nextMsg = KEYP1;
			else if(players == 2) init.nextMsg = KEYP2;

			i = msgsnd(id, &init, sizeof(init.nextMsg), IPC_NOWAIT);
			if(i == -1) perror("msgsnd error");
			else printf("Init #%d sent back\n", initNr);

			initNr++;
		}
		if(players == 2) doBreak = 0;
	}


	state->resources[0] = 300;
	Data data = sendGameState(state, 0);
	i = msgsnd(idP1, &data, sizeof(Data) - sizeof(data.mtype), IPC_NOWAIT);
	if(i == -1) perror("msgsnd error");
	else printf("Update sent to player #1\n");



	sleep(3);

	int destructor = msgctl(id, IPC_RMID, 0);
	if(destructor == -1) perror("destructor error");
	else printf("Queue #1 key = %d destructed\n", KEY);

	destructor = msgctl(idP1, IPC_RMID, 0);
	if(destructor == -1) perror("destructor error");
	else printf("Queue #2 key = %d destructed\n", KEYP1);

	destructor = msgctl(idP2, IPC_RMID, 0);
	if(destructor == -1) perror("destructor error");
	else printf("Queue #3 key = %d destructed\n", KEYP2);

	shmdt(state);
}
