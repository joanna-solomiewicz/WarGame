#include "headers/server.h"

int main() {
	
	initConsts();
	initStateMemory();
	initSemaphore();
	initQueues();
	signal(SIGINT, f);
	signal(SIGQUIT, f);
	signal(SIGTERM, f);
	initConnection();
	waitingForPlayers();
	initData();
	heartbeat();

	while(1) {
		sendResources();
		receiveBuild();
		receiveAttack();
		clear();
		printGameState();
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

AttackForce setAttackForce() {
	AttackForce attackForce;
	attackForce.light = 1;
	attackForce.heavy = 1.5;
	attackForce.cavalry = 3.5;
	return attackForce;
}

DefenceForce setDefenceForce() {
	DefenceForce defenceForce;
	defenceForce.light = 1.2;
	defenceForce.heavy = 3;
	defenceForce.cavalry = 1.2;
	return defenceForce;
}

void initConsts() {
	prices = setPrices();
	attackForce = setAttackForce();
	defenceForce = setDefenceForce();
	beatSkips[0] = 0;
	beatSkips[1] = 0;
}

void initStateMemory() {
	/* Creating type State* shared memory */
	key_t key = KEYMEM;
	int shmid = shmget(key, sizeof(State), IPC_CREAT | 0640);
	if(shmid == -1) perror("shmget error");
	state = (State*)shmat(shmid, NULL, 0);
}

void initSemaphore() {
	int semid = semget(KEYSEM, 1, IPC_CREAT | 0640);
	if(semid == -1) perror("semget error");
	else {
		int j = semctl(semid, 0, SETVAL, 1);
		if(j == -1) perror("semctl error");
	}
	semaphoreID = semid;
}

void P(int semid, struct sembuf sem) {
	sem.sem_num = 0;
	sem.sem_op = -1;
	sem.sem_flg = 0;
	int i = semop(semid, &sem, 1);
	if(i == -1) perror("semop error");
}

void V(int semid, struct sembuf sem) {
	sem.sem_num = 0;
	sem.sem_op = 1;
	sem.sem_flg = 0;
	int i = semop(semid, &sem, 1);
	if(i == -1) perror("semop error");
}

void clear() { printf("\033[H\033[J"); }

void printGameState() {
	printf(	"\n\t\t\t  WAR GAME STATE"
				"\n\t\tP1\t\t\t\tP2"
				"\n\t~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
				"\n\tlight\t\t%d\t|\tlight\t\t%d"
				"\n\theavy\t\t%d\t|\theavy\t\t%d"
				"\n\tcavalry\t\t%d\t|\tcavalry\t\t%d"
				"\n\tworkers\t\t%d\t|\tworkers\t\t%d"
				"\n\tpoints\t\t%d\t|\tpoints\t\t%d"
				"\n\tresources\t%d\t|\tresources\t%d\n",
				state->light[0], state->light[1], state->heavy[0], state->heavy[1], state->cavalry[0], state->cavalry[1], 
				state->workers[0], state->workers[1], state->points[0], state->points[1], state->resources[0], state->resources[1]);
}

void sendGameState(int player, char info[]) {
	Data data;
	if(!player) data.mtype = 1;
	else data.mtype = 1;
	data.light = state->light[player];
	data.heavy = state->heavy[player];
	data.cavalry = state->cavalry[player];
	data.workers = state->workers[player];
	data.points = state->points[player];
	data.resources = state->resources[player];
	strcpy(data.info, info);
	data.end = state->end;
	
	int i;
	if(!player) i = msgsnd(queueIdList->player1Q, &data, sizeof(Data) - sizeof(data.mtype), IPC_NOWAIT);
	else i = msgsnd(queueIdList->player2Q, &data, sizeof(Data) - sizeof(data.mtype), IPC_NOWAIT);
	if(i == -1) perror("msgsnd error");
}

void f() {
	printf("\nI've been killed !\n");
	destruction();
	int i = kill(0, SIGKILL);
	if(i == -1) perror("kill error");

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
//		printf("Another server already working\n");
//		exit(0);
		del = msgctl(id, IPC_RMID, 0);
		if(del == -1) perror("msgctl error");
		id = msgget(key, IPC_CREAT | 0640);
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
	printf("The war shall begin\n");
}

void initData() {
	/* Sending 300 entities of resources to players */
	int player;
	P(semaphoreID, sem);
	for(player = 0; player < 2; player++) {
		state->light[player] = 0;
		state->heavy[player] = 0;
		state->cavalry[player] = 0;
		state->workers[player] = 0;
		state->points[player] = 0;
		state->resources[player] = 300;
		state->end = 0;
		
		if(!player) sendGameState(player, "I'M PLAYER #1\0");
		else sendGameState(player, "I'M PLAYER #2\0");
	}
	V(semaphoreID, sem);
}

void heartbeat() {

	Alive alive;
	int player;

	int f = fork();
	if(f == 0) player = 0;
	else {
		f = fork();
		if(f == 0) player = 1;
		else return;
	}

	while(1) {
		sleep(2);

		alive.mtype = 5;
		alive.lol = 's';
		int i,j;
		int type = 4;
		if(!player) {
			i = msgsnd(queueIdList->player1Q, &alive, sizeof(alive.lol), IPC_NOWAIT);
			if(i == -1) perror("msgsnd error");
			j = msgrcv(queueIdList->player1Q, &alive, sizeof(alive.lol), type, IPC_NOWAIT);
		}
		else {
			i = msgsnd(queueIdList->player2Q, &alive, sizeof(alive.lol), IPC_NOWAIT);
			if(i == -1) perror("msgsnd error");
			j = msgrcv(queueIdList->player2Q, &alive, sizeof(alive.lol), type, IPC_NOWAIT);
		}

		if(j == -1) beatSkips[player]++; 
		else beatSkips[player] = 0; 

		if(beatSkips[player] >= 3) { 
			P(semaphoreID, sem);
			state->end = '1';
			V(semaphoreID, sem);
			sendGameState(0, "END OF WAR\0");
			sendGameState(1, "END OF WAR\0");
			destruction();
			int k = kill(0, SIGKILL);
			if(k == -1) perror("kill error");
			exit(0);
		}
	}
}

void sendResources() {
	sleep(1);
	int player;
	int p1 = fork();
	if(p1 == 0) player = 0;
	else {
		int p2 = fork();
		if(p2 == 0) player = 1;
		else return;
	}
	P(semaphoreID, sem);
	state->resources[player] += 50 + state->workers[player]*5;
	V(semaphoreID, sem);
	sendGameState(player, "\0");
	exit(0);
}

void receiveBuild() {
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
				P(semaphoreID, sem);
				state->resources[player] -= build.light*prices.light;
				V(semaphoreID, sem);
				printBuild(build, player);
				int i;
				for(i = build.light; i > 0; i--) {
					sleep(2);
					P(semaphoreID, sem);
					state->light[player]++;
					V(semaphoreID, sem);
					sendGameState(player, "Light warrior recruited\0");
					printf("Light warrior has been recruited\n");
//					printGameState(state);
				}
			}
			else { 
				sendGameState(player, "Not enough resources to recruit\0");
				printf("Player #%d, not enough resources\n", player+1); 
			}
		}
		else if(build.heavy) {
			if(build.heavy*prices.heavy <= state->resources[player]) {
				P(semaphoreID, sem);
				state->resources[player] -= build.heavy*prices.heavy;
				V(semaphoreID, sem);
				printBuild(build, player);
				int i;
				for(i = build.heavy; i > 0; i--) {
					sleep(3);
					P(semaphoreID, sem);
					state->heavy[player]++;
					V(semaphoreID, sem);
					sendGameState(player, "Heavy warrior recruited\0");
					printf("Heavy warrior has been recruited\n");
//					printGameState(state);
				}
			}
			else { 
				sendGameState(player, "Not enough resources to recruit\0");
				printf("Player #%d, not enough resources\n", player+1); 
			}
		}
		else if(build.cavalry) {
			if(build.cavalry*prices.cavalry <= state->resources[player]) {
				P(semaphoreID, sem);
				state->resources[player] -= build.cavalry*prices.cavalry;
				V(semaphoreID, sem);
				printBuild(build, player);
				int i;
				for(i = build.cavalry; i > 0; i--) {
					sleep(5);
					P(semaphoreID, sem);
					state->cavalry[player]++;
					V(semaphoreID, sem);
					sendGameState(player, "Cavalryman recruited\0");
					printf("Cavalryman has been recruited\n");
//					printGameState(state);
				}
			}
			else { 
				sendGameState(player, "Not enough resources to recruit\0");
				printf("Player #%d, not enough resources\n", player+1); 
			}
		}
		else if(build.workers) {
			if(build.workers*prices.workers <= state->resources[player]) {
				P(semaphoreID, sem);
				state->resources[player] -= build.workers*prices.workers;
				V(semaphoreID, sem);
				printBuild(build, player);
				int i;
				for(i = build.workers; i > 0; i--) {
					sleep(2);
					P(semaphoreID, sem);
					state->workers[player]++;
					V(semaphoreID, sem);
					sendGameState(player, "Worker recruited\0");
					printf("Worker has been recruited\n");
//					printGameState(state);
				}
			}
			else { 
				sendGameState(player, "Not enough resources to recruit\0");
				printf("Player #%d, not enough resources\n", player+1); 
			}
		}
	}
	exit(0);	
}

void printBuild(Build build, int player) {
	printf("Player #%d: %d light, %d heavy, %d cavalry, %d workers\n", 
				player+1, build.light, build.heavy, build.cavalry, build.workers);
}

void receiveAttack() {
	int type = 3;
	int player;

	int p1 = fork();
	if(p1 == 0) player = 0;
	else {
		int p2 = fork();
		if(p2 == 0) player = 1;
		else return;
	}

	Attack attack;
	int i;
	if(!player) i = msgrcv(queueIdList->player1Q, &attack, sizeof(Attack) - sizeof(attack.mtype), type, IPC_NOWAIT);
	else i = msgrcv(queueIdList->player2Q, &attack, sizeof(Attack) - sizeof(attack.mtype), type, IPC_NOWAIT);
	if(i != -1) {
		/* Subtract the attacking warriors */
		P(semaphoreID, sem);
		state->light[player] -= attack.light;
		state->heavy[player] -= attack.heavy;
		state->cavalry[player] -= attack.cavalry;
		V(semaphoreID, sem);
		printf("Attack received from player #%d\n", player+1);
		sleep(5);
		int enemy = (player+1)%2;
		int aAttack = attack.light*attackForce.light + attack.heavy*attackForce.heavy + attack.cavalry*attackForce.cavalry;
		int aDefence = attack.light*defenceForce.light + attack.heavy*defenceForce.heavy + attack.cavalry*defenceForce.cavalry;
		int dAttack = state->light[enemy]*attackForce.light + state->heavy[enemy]*attackForce.heavy + state->cavalry[enemy]*attackForce.cavalry;
		int dDefence = state->light[enemy]*defenceForce.light + state->heavy[enemy]*defenceForce.heavy + state->cavalry[enemy]*defenceForce.cavalry;

		if(aAttack-dDefence > 0) {
			P(semaphoreID, sem);
			state->light[enemy] = state->heavy[enemy] = state->cavalry[enemy] = 0;
			V(semaphoreID, sem);
			printf("Player #%d has won the battle\n", player+1);
			P(semaphoreID, sem);
			state->points[player]++;
			state->light[player] += attack.light;
			state->heavy[player] += attack.heavy;
			state->cavalry[player] += attack.cavalry;
			V(semaphoreID, sem);
		}
		else {
			P(semaphoreID, sem);
			state->light[enemy] -= state->light[enemy]*aAttack/dDefence;
			state->heavy[enemy] -= state->heavy[enemy]*aAttack/dDefence;
			state->cavalry[enemy] -= state->cavalry[enemy]*aAttack/dDefence;
			V(semaphoreID, sem);
		}
		if(dAttack-aDefence > 0) state->light[player] = state->heavy[player] = state->cavalry[player] = 0;
		else {
			P(semaphoreID, sem);
			state->light[player] += attack.light - attack.light*dAttack/aDefence;
			state->heavy[player] += attack.heavy - attack.heavy*dAttack/aDefence;
			state->cavalry[player] += attack.cavalry - attack.cavalry*dAttack/aDefence;
			V(semaphoreID, sem);
		}
		printf("End of attack by player #%d\n", player+1);
//		printGameState(state);
	}
	exit(0);
}

void destruction() {
	/* Destruction */
	int destructor = msgctl(queueIdList->initialQ, IPC_RMID, 0);
	if(destructor == -1) perror("destructor error");
//	else printf("Queue #1 key = %d destructed\n", KEY);

	destructor = msgctl(queueIdList->player1Q, IPC_RMID, 0);
	if(destructor == -1) perror("destructor error");
//	else printf("Queue #2 key = %d destructed\n", KEYP1);

	destructor = msgctl(queueIdList->player2Q, IPC_RMID, 0);
	if(destructor == -1) perror("destructor error");
//	else printf("Queue #3 key = %d destructed\n", KEYP2);

	shmdt(state);
}

