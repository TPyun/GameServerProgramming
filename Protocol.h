#pragma once
#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>

#pragma comment (lib, "WS2_32.LIB")
#pragma comment(lib, "MSWSock.lib")

#define WIDTH 800
#define HEIGHT 800

#define CS_LOGIN 0
#define SC_MOVE 1
#define CS_MOVE 2

typedef struct two_ints {
	int x;
	int y;
} TI;

typedef struct two_floats {
	float x;
	float y;
} TF;

typedef struct key_state {
	bool up;
	bool down;
	bool left;
	bool right;
} KS;

struct CS_LOGIN_PACKET {
	unsigned char size = sizeof(CS_LOGIN_PACKET);
	unsigned char type = CS_LOGIN;
};

struct SC_MOVE_PACKET {
	unsigned char size = sizeof(SC_MOVE_PACKET);
	unsigned char type = SC_MOVE;

	int client_id;
	TI position;
};

struct CS_MOVE_PACKET {
	unsigned char size = sizeof(CS_MOVE_PACKET);
	unsigned char type = CS_MOVE;

	KS ks;
};