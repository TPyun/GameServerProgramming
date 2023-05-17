#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>

#pragma comment (lib, "WS2_32.LIB")
#pragma comment(lib, "MSWSock.lib")

#define SERVER_PORT 9000

constexpr int BUFSIZE = 200;
constexpr int MAX_USER = 2100;
constexpr int MAX_NPC = 200000;

constexpr int WIDTH = 750;	//Client
constexpr int HEIGHT = 750;	//Client

constexpr int MAP_SIZE = 2000;		//Server

constexpr int CLIENT_RANGE = 11;		//Client
constexpr int VIEW_RANGE = (CLIENT_RANGE - 1) / 2;	//Server

constexpr int BLOCK_SIZE = WIDTH / CLIENT_RANGE;	//Both

constexpr int MAX_CHAT = 100;

constexpr char P_CS_LOGIN = 0;
constexpr char P_CS_MOVE = 1;
constexpr char P_CS_CHAT = 2;

constexpr char P_SC_LOGIN = 64;
constexpr char P_SC_MOVE = 65;
constexpr char P_SC_OUT = 66;
constexpr char P_SC_CHAT = 67;


typedef struct two_ints {
	int x;
	int y;
} TI;

typedef struct two_uints {
	unsigned int x;
	unsigned int y;
} TUI;

typedef struct two_uchar {
	unsigned char x;
	unsigned char y;
} TUC;


typedef struct two_floats {
	float x;
	float y;
} TF;

typedef struct two_ushorts {
	unsigned short x;
	unsigned short y;
} TUS;

typedef struct key_state {
	bool up = false;
	bool down = false;
	bool left = false;
	bool right = false;
} KS;


#pragma pack (push, 1)
//CLIENT TO SERVER
struct CS_LOGIN_PACKET {
	unsigned char size = sizeof(CS_LOGIN_PACKET);
	unsigned char type = P_CS_LOGIN;
};
struct CS_MOVE_PACKET {
	unsigned char size = sizeof(CS_MOVE_PACKET);
	unsigned char type = P_CS_MOVE;

	KS ks;
	unsigned int time{};
};
struct CS_CHAT_PACKET {
	unsigned char size = sizeof(CS_CHAT_PACKET);
	unsigned char type = P_CS_CHAT;

	char message[MAX_CHAT]{};
};

//SERVER TO CLIENT
struct SC_LOGIN_PACKET {
	unsigned char size = sizeof(SC_LOGIN_PACKET);
	unsigned char type = P_SC_LOGIN;
	
	int client_id;
	TI position;
};
struct SC_MOVE_PACKET {
	unsigned char size = sizeof(SC_MOVE_PACKET);
	unsigned char type = P_SC_MOVE;

	int client_id;
	TI position;
	unsigned int time{};
};
struct SC_OUT_PACKET {
	unsigned char size = sizeof(SC_OUT_PACKET);
	unsigned char type = P_SC_OUT;

	int client_id;
};
struct SC_CHAT_PACKET {
	unsigned char size = sizeof(SC_CHAT_PACKET);
	unsigned char type = P_SC_CHAT;

	int client_id;
	char message[MAX_CHAT]{};
};

#pragma pack (pop)