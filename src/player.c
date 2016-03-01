#include "headers/player.h"

int main() {

	nonblock(NB_ENABLE);
	
	beatSkips = 0;

	strcpy(status, "");
	int l;
	for(l = 0; l < 4; l++) strcpy(infoLog[l], "");
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
	strcpy(status, data.info);
	printGameState(data);
	printLog();

	/* Printing game state with terminal clearing */
	update(data);
	heartbeat(id2);
	while(1) {
		char c;
		if( kbhit() ) {
			c = getchar();
			if(c == '1') building(data, id2);
			else if(c == '2') attacking(data, id2);
		}
		usleep(1);
		i = msgrcv(id2, &data, sizeof(Data) - sizeof(data.mtype), type, 0);
		if(i != -1) {
			updateLog(data);
			update(data);
		}
	}

	nonblock(NB_DISABLE);
}




void clear() { printf("\033[H\033[J"); }

void printGameState(Data data) {
	printf("\n\t  %s\n\tMY WAR GAME STATE\n\tlight:\t\t%d\n\theavy:\t\t%d\n\tcavalry:\t%d\n\tworkers:\t%d\n\tpoints:\t\t%d\n\tresources:\t%d\n", status, data.light, data.heavy, data.cavalry, data.workers, data.points, data.resources);
}

void updateLog(Data data) {
	if(strlen(data.info) > 1) {
		strcpy(infoLog[0], infoLog[1]);
		strcpy(infoLog[1], infoLog[2]);
		strcpy(infoLog[2], infoLog[3]);
		strcpy(infoLog[3], data.info);
	}
}

void printLog() {
	printf("\n\t~~ %s\n\t~~ %s\n\t~~ %s\n\t~~ %s\n\n", infoLog[0], infoLog[1], infoLog[2], infoLog[3]);
}

void printMenu() { printf("\t[1] BUILD [2] ATTACK\n"); }

void sendBuildMessage(Build build, int id2) {
	int i = msgsnd(id2, &build, sizeof(Build) - sizeof(build.mtype), 0);
	if(i == -1) perror("msgrcv error");
}

/* Previous building version */
void building(Data data, int id2) { 
	update(data);
	printf("\n\t[1] LIGHT\n\t[2] HEAVY\n\t[3] CAVALRY\n\t[4] WORKERS\n\t[5] NOTHING, hit the wrong button\n");
	int stop = 1;
	while(stop) {
		char c;
		c = getchar();
		if( c >= '0' && c <= '4') {
			printf("\n\tHOW MANY? (0-9)\n");
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

		update(data);
		usleep(1);
	}
}

void sendAttackMessage(Attack attack, int id2) {
	int i = msgsnd(id2, &attack, sizeof(Attack) - sizeof(attack.mtype), 0);
	if(i == -1) perror("msgrcv error");
}

void attacking(Data data, int id2) {
	update(data);
//	printf("\n\tHow many warriors would You like to send to war?\n");

	Attack attack;
	attack.mtype = 3;
	attack.light = attack.heavy = attack.cavalry = 0;
	int i;
	for(i = 0; i < 3; i++) {
		switch(i) {
			case 0:
				printf("\n\tHOW MANY LIGHT WARRIORS? (0-9)\n");
				break;
			case 1:
				printf("\tHOW MANY HEAVY WARRIORS? (0-9)\n");
				break;
			case 2:
				printf("\tHOW MANY CAVALRYMAN? (0-9)\n");
				break;
		}
		int stop = 1;
		while(stop) {
			char c;
			c = getchar();
			if(c >= '0' && c <= '9') {
				switch(i) {
					case 0:
						attack.light = (int)c-48;
						break;
					case 1:
						attack.heavy = (int)c-48;
						break;
					case 2:
						attack.cavalry = (int)c-48;
						break;
				}
				stop = 0;
			}
		}
	}
	sendAttackMessage(attack, id2);
	update(data);
	usleep(1);
}

void update(Data data) {
	clear();
	printGameState(data);
	printLog();
	printMenu();
}

void heartbeat(int id2) {
	int f = fork();
	if(f != 0) return;

	Alive alive;
	while(1) {
		sleep(2);

		alive.mtype = 4;
		int i = msgsnd(id2, &alive, 0, IPC_NOWAIT);
//		if(i == -1) perror("msgsnd error");

		int type = 5;
		i = msgrcv(id2, &alive, 0, type, IPC_NOWAIT);
		if(i == -1) beatSkips++;

		if(beatSkips >= 3) { 
			int k = kill(0, SIGKILL);
			if(k == -1) perror("kill error");
			exit(0);
		}
	}
}
