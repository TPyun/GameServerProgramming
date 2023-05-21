#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>

#pragma comment (lib, "WS2_32.LIB")
#pragma comment(lib, "MSWSock.lib")

#define SERVER_PORT 9000

constexpr int BUFSIZE = 200;
constexpr int MAX_USER = 100000;
constexpr int MAX_NPC = 200000;

constexpr int WIDTH = 735;	//Client
constexpr int HEIGHT = 735;	//Client

constexpr int MAP_SIZE = 2000;		//Server

constexpr int CLIENT_RANGE = 21;		//Client
constexpr int VIEW_RANGE = 7;	//Server
constexpr int SECTOR_SIZE = VIEW_RANGE * 2 + 1;	//Server
constexpr int SECTOR_NUM = MAP_SIZE / SECTOR_SIZE + 1;	//Server

constexpr int BLOCK_SIZE = WIDTH / CLIENT_RANGE;	//Both

constexpr int MAX_CHAT = 100;

constexpr char P_CS_LOGIN = 0;
constexpr char P_CS_MOVE = 1;
constexpr char P_CS_CHAT = 2;
constexpr char P_CS_ATTACK = 3;
constexpr char P_CS_DIRECTION = 4;

constexpr char P_SC_LOGIN_INFO = 64;
constexpr char P_SC_MOVE = 65;
constexpr char P_SC_IN = 66;
constexpr char P_SC_OUT = 67;
constexpr char P_SC_CHAT = 68;
constexpr char P_SC_ATTACK = 69;
constexpr char P_SC_DIRECTION = 70;
constexpr char P_SC_STAT_CHANGE = 71;


typedef struct two_ints {
	int x;
	int y;
} TI;

typedef struct two_uints {
	unsigned int x;
	unsigned int y;
} TUI;

typedef struct two_char {
	char x;
	char y;
} TC;

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
	char type = P_CS_LOGIN;

	char name[30];
};
struct CS_MOVE_PACKET {
	unsigned char size = sizeof(CS_MOVE_PACKET);
	char type = P_CS_MOVE;

	KS ks;
	unsigned int time{};
};
enum Direction { DIR_UP, DIR_LEFT, DIR_DOWN, DIR_RIGHT };
struct CS_DIRECTION_PACKET {
	unsigned char size = sizeof(CS_DIRECTION_PACKET);
	char type = P_CS_DIRECTION;

	Direction direction;
};
struct CS_CHAT_PACKET {
	unsigned char size = sizeof(CS_CHAT_PACKET);
	char type = P_CS_CHAT;

	char message[MAX_CHAT]{};
};
struct CS_ATTACK_PACKET {
	unsigned char size = sizeof(CS_ATTACK_PACKET);
	char type = P_CS_ATTACK;

	unsigned int time{};
};

//SERVER TO CLIENT
struct SC_LOGIN_INFO_PACKET {
	unsigned char size = sizeof(SC_LOGIN_INFO_PACKET);
	char type = P_SC_LOGIN_INFO;
	
	int client_id;
	char name[30]{};
	TI position;
	int	hp;
	int	max_hp;
	int	exp;
	int	level;
};
struct SC_MOVE_PACKET {
	unsigned char size = sizeof(SC_MOVE_PACKET);
	char type = P_SC_MOVE;

	int client_id;
	TI position;
	unsigned int time{};
};
struct SC_DIRECTION_PACKET {
	unsigned char size = sizeof(SC_DIRECTION_PACKET);
	char type = P_SC_DIRECTION;

	int client_id;
	Direction direction;
};
struct SC_IN_PACKET {
	unsigned char size = sizeof(SC_IN_PACKET);
	char type = P_SC_IN;

	int client_id;
	char name[30]{};
	TI position;
};
struct SC_OUT_PACKET {
	unsigned char size = sizeof(SC_OUT_PACKET);
	char type = P_SC_OUT;

	int client_id;
};
struct SC_CHAT_PACKET {
	unsigned char size = sizeof(SC_CHAT_PACKET);
	char type = P_SC_CHAT;

	int client_id;
	char message[MAX_CHAT]{};
};
struct SC_ATTACK_PACKET {
	unsigned char size = sizeof(SC_ATTACK_PACKET);
	char type = P_SC_ATTACK;

	int client_id;
};
struct SC_STAT_CHANGE_PACKET {
	unsigned char size = sizeof(SC_STAT_CHANGE_PACKET);
	char type = P_SC_STAT_CHANGE;
	
	int	hp;
	int	max_hp;
	int	exp;
	int	level;
};

#pragma pack (pop)