#include "headers/struct.h"
#include "headers/keyHandling.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <unistd.h>

void clear();
void printGameState(Data data);
void printMenu();
void building(Data data);

int main() {

	nonblock(NB_ENABLE);

	key_t key = KEY;

	int id = msgget(key, IPC_CREAT | 0640);
	if(id == -1) perror("msgget error");

	/* Sending type 1 message to server */
	Init init;
	init.mtype = 1;
	init.nextMsg = getpid();
	int i = msgsnd(id, &init, sizeof(init.nextMsg), 0);
	if(i == -1) perror("msgsnd error");
	else printf("Init sent\n");

	/* Waiting for type 2 message from server containing a key for a private msg queue */
	long type = 2;
	i = msgrcv(id, &init, sizeof(init.nextMsg), type, 0);
	if(i == -1) perror("msgrcv error");
	else printf("Init received %d\n", init.nextMsg);

	/* Opening the private queue */
	key = init.nextMsg;
	int id2 = msgget(key, 0640);
	if(id2 == -1) perror("msgget error");
	else printf("Queue opened\n");

	/* Receiving game state */
	Data data;
	type = key;
	i = msgrcv(id2, &data, sizeof(Data) - sizeof(data.mtype), type, 0);
	if(i == -1) perror("msgrcv error");
	else printf("Update received\n");
	printGameState(data);

	/* Printing game state with terminal clearing */
	clear();
	printGameState(data);
	printMenu();
	while(1) {
		char c;
		if( kbhit() ) {
			c = getchar();
			if(c == '1') building(data);
			else if(c == '2') {
				clear();
				printGameState(data);
				printMenu();
				printf("Attacking!\n");
			}
		}
		usleep(1);
	}

	nonblock(NB_DISABLE);
}

void clear() { printf("\033[H\033[J"); }

void printGameState(Data data) {
	printf("Game state:\n\tlight: %d\n\theavy: %d\n\tcavalry: %d\n\tworkers: %d\n\tpoints: %d\n\tresources: %d\n\tinfo: %s\n\tend: %c\n", 
							data.light, data.heavy, data.cavalry, data.workers, data.points, data.resources, data.info, data.end);
}

void printMenu() { printf("[1] BUILD [2] ATTACK\n"); }

void building(Data data) { 
	clear();
	printGameState(data);
	printf("What do You want to build? [1] LIGHT [2] HEAVY [3] CAVALRY [4] WORKERS [5] NOTHING, hit the wrong button\n");
	int stop = 1;
	while(stop) {
		char c;
		int nr;
		c = getchar();
		if( c >= '0' && c <= '4') {
			printf("How many? [ENTER] Confirm\n");
			nonblock(NB_DISABLE);
			printf("~~Type a magical, invisible number~~\n");
			scanf("%d", &nr);
			nonblock(NB_ENABLE);
			stop = 0;
//			while(stop) {
//				if( kbhit() ) {
//					c = getchar();
//					if((int)c == 13) { stop = 0; }
//				}
//			}
		}
		else if( c == '5' ) { stop = 0; }

		clear();
		printGameState(data);
		printMenu();

		usleep(1);
	}
}
