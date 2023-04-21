#pragma once
#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>

#pragma comment (lib, "WS2_32.LIB")
#pragma comment(lib, "MSWSock.lib")

#define SERVER_PORT 9000

constexpr int WIDTH = 750;	//pixels
constexpr int HEIGHT = 750;	//pixels

constexpr int MAP_SIZE = 400;		//blocks
constexpr int VIEW_RANGE = 7;	//blocks

constexpr int DISTANCE = 15;		//blocks
constexpr int BLOCK_SIZE = WIDTH / DISTANCE;	//pixels

constexpr int BUFSIZE = 200;
constexpr int MAX_USER = 10000;

#define CS_LOGIN 0
#define SC_LOGIN 1

#define SC_MOVE 2
#define CS_MOVE 3

#define SC_OUT 4

typedef struct two_ints {
	int x;
	int y;
} TI;

typedef struct two_floats {
	float x;
	float y;
} TF;

typedef struct two_shorts {
	unsigned short x;
	unsigned short y;
} TS;

typedef struct key_state {
	bool up = false;
	bool down = false;
	bool left = false;
	bool right = false;
} KS;

struct CS_LOGIN_PACKET {
	unsigned char size = sizeof(CS_LOGIN_PACKET);
	unsigned char type = CS_LOGIN;
};

struct SC_LOGIN_PACKET {
	unsigned char size = sizeof(SC_LOGIN_PACKET);
	unsigned char type = SC_LOGIN;
	
	int client_id;
	TS position;
};

struct SC_MOVE_PACKET {
	unsigned char size = sizeof(SC_MOVE_PACKET);
	unsigned char type = SC_MOVE;

	int client_id;
	TS position;
	unsigned int time{};
};

struct CS_MOVE_PACKET {
	unsigned char size = sizeof(CS_MOVE_PACKET);
	unsigned char type = CS_MOVE;

	KS ks;
	unsigned int time{};
};

struct SC_OUT_PACKET {
	unsigned char size = sizeof(SC_OUT_PACKET);
	unsigned char type = SC_OUT;

	int client_id;
};