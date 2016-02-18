#pragma once

#define KEY 15071410 

typedef struct Init {
	long mtype;
	int nextMsg;
} Init;

typedef struct Data {
	long mtype;
	int light;
	int heavy;
	int cavalry;
	int workers;
	int points;
	int resources;
	char info[120]; //wysylajmy tylko tekst, bez ‘\n’ na koncu
	char end; //jak ktos zwyciezy to to rozne od 0
} Data;

typedef struct Build {
	long mtype;
	int light;
	int heavy;
	int cavalry;
	int workers;
} Build;

typedef struct Attack {
	long mtype;
	int light;
	int heavy;
	int cavalry;
} Attack;

typedef struct Alive {
	long mtype;
} Alive;
