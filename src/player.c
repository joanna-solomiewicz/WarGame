#include "headers/struct.h"
#include "headers/keyHandling.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <unistd.h>

void printGameState(Data data) {
	printf("Game state:\n\tlight: %d\n\theavy: %d\n\tcavalry: %d\n\tworkers: %d\n\tpoints: %d\n\tresources: %d\n\tinfo: %c\n\tend: %c\n", 
							data.light, data.heavy, data.cavalry, data.workers, data.points, data.resources, data.info, data.end);
}

void clear() {
	printf("\033[H\033[J");
}

int main() {

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
	while(1) {
		char c;
		nonblock(NB_ENABLE);
		if( kbhit() ) {
			clear();
			printGameState(data);
			c = getchar();
			printf("%c\n", c);
		}
		nonblock(NB_DISABLE);
		usleep(1);
	}

}
