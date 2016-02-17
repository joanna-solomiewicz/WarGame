#include "headers/struct.h"
#include "headers/keyHandling.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void clear();
void printGameState(Data data);
void printMenu();
void sendBuildMessage(Build build, int id2);
void building(Data data, int id2);
void sendAttackMessage(Attack attack, int id2);
void attacking(Data data, int id2);
void update(Data data);

int main() {

	nonblock(NB_ENABLE);

	key_t key = KEY;

	int id = msgget(key, 0640);
	if(id == -1) {
		printf("Server is not active. Come back later ;)\n");
		exit(0);
	}

	/* Waiting for type 1 message from server containing a key for a private msg queue */
	Init init;
	long type = 1;
	int i = msgrcv(id, &init, sizeof(init.nextMsg), type, IPC_NOWAIT);
	if(i == -1) {
		printf("Server is not available. Come back later ;)\n");
		exit(0);
	}
	else {
		printf("Init received %d\n", init.nextMsg);
		Init initBack;
		initBack.mtype = 2;
		i = msgsnd(id, &initBack, sizeof(init.nextMsg), 0);
		if(i == -1) perror("msgsnd error");
		else printf("To server, private key received\n");
	}

	/* Opening the private queue */
	key = init.nextMsg;
	int id2 = msgget(key, 0640);
	if(id2 == -1) perror("msgget error");
	else printf("Queue opened\n");

	/* Receiving game state */
	Data data;
	type = 1;
	do {
		i = msgrcv(id2, &data, sizeof(Data) - sizeof(data.mtype), type, 0);
	} while(i == -1);
	printf("Update received\n");
	printGameState(data);

	/* Printing game state with terminal clearing */
	clear();
	printGameState(data);
	printMenu();
	while(1) {
		char c;
		if( kbhit() ) {
			c = getchar();
			if(c == '1') building(data, id2);
			else if(c == '2') {
				attacking(data, id2);
//				printf("Attacking!\n");
				sleep(5);
			}
		}
		usleep(1);
		i = msgrcv(id2, &data, sizeof(Data) - sizeof(data.mtype), type, 0);
//		if(i == -1) perror("msgrcv error");
//		else update(data);
		if(i != -1) update(data);
	}

	nonblock(NB_DISABLE);
}

void clear() { printf("\033[H\033[J"); }

void printGameState(Data data) {
	printf("Game state:\n\tlight: %d\n\theavy: %d\n\tcavalry: %d\n\tworkers: %d\n\tpoints: %d\n\tresources: %d\n\tinfo: %s\n\tend: %c\n", 
							data.light, data.heavy, data.cavalry, data.workers, data.points, data.resources, data.info, data.end);
}

void printMenu() { printf("[1] BUILD [2] ATTACK\n"); }

void sendBuildMessage(Build build, int id2) {
	int i = msgsnd(id2, &build, sizeof(Build) - sizeof(build.mtype), 0);
	if(i == -1) perror("msgrcv error");
	else printf("Build sent to server:\n\tlight: %d\n\theavy: %d\n\tcavalry: %d\n\tworkers: %d\n",
							build.light, build.heavy, build.cavalry, build.workers);
	sleep(1);
}

void building(Data data, int id2) { 
	update(data);
	printf("What do You want to build? [1] LIGHT [2] HEAVY [3] CAVALRY [4] WORKERS [5] NOTHING, hit the wrong button\n");
	int stop = 1;
	while(stop) {
		char c;
		c = getchar();
		if( c >= '0' && c <= '4') {
			printf("How many? (0-9)\n");
			while(stop) {
				if( kbhit() ) {
					char ch = getchar();
					if(ch >= '0' && ch <= '9') { 
						Build build;
						build.mtype = 2;
						build.light = build.heavy = build.cavalry = build.workers = 0;
						switch((int)c-48) {
							case 1:
								build.light = (int)ch-48;
								break;
							case 2:
								build.heavy = (int)ch-48;
								break;
							case 3:
								build.cavalry = (int)ch-48;
								break;
							case 4:
								build.workers = (int)ch-48;
								break;
						}
						sendBuildMessage(build, id2);
						stop = 0; 
					}
				}
			}
		}
		else if( c == '5' ) { stop = 0; }

		clear();
		printGameState(data);
		printMenu();

		usleep(1);
	}
}

void sendAttackMessage(Attack attack, int id2) {
	int i = msgsnd(id2, &attack, sizeof(Attack) - sizeof(attack.mtype), 0);
	if(i == -1) perror("msgrcv error");
	else printf("Attack sent to server:\n\tlight: %d\n\theavy: %d\n\tcavalry: %d\n",
							attack.light, attack.heavy, attack.cavalry);
	sleep(1);
}

void attacking(Data data, int id2) {
	Attack attack;
	attack.mtype = 3;
	attack.light = attack.heavy = attack.cavalry = 1;
	sendAttackMessage(attack, id2);
}

void update(Data data) {
	clear();
	printGameState(data);
	printMenu();
}
