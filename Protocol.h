#pragma once
#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>

#pragma comment (lib, "WS2_32.LIB")
#pragma comment(lib, "MSWSock.lib")
constexpr int PORT_NUM = 9000;
constexpr int CHAT_SIZE = 100;
constexpr int NAME_SIZE = 20;
constexpr int BUFSIZE = 200;

constexpr int MAX_USER = 51000;
constexpr int MAX_NPC = 200000;
constexpr int MAX_OBSTACLE = 500000;

constexpr int USER_START = 0;
constexpr int NPC_START = USER_START + MAX_USER;
constexpr int NORMAL_NPC_START = USER_START + MAX_USER; 
constexpr int AGGR_NPC_START = USER_START + MAX_USER + MAX_NPC * 3 / 4;
constexpr int OBSTACLE_START = NPC_START + MAX_NPC;
constexpr int AGGR_RANGE = 5;

constexpr int WIDTH = 760;	//Client
constexpr int HEIGHT = 760;	//Client

constexpr int W_WIDTH = 2000;		//Size of Map
constexpr int W_HEIGHT = 2000;

constexpr int CLIENT_RANGE = 19;		//Client
constexpr int VIEW_RANGE = 7;	//Server
constexpr int SECTOR_SIZE = VIEW_RANGE * 2 + 1;	//Server
constexpr int SECTOR_NUM = W_WIDTH / SECTOR_SIZE + 1;	//Server
constexpr int BLOCK_SIZE = WIDTH / CLIENT_RANGE;	//Both

constexpr int PLAYER_MOVE_TIME = 1000;	//모두 1000으로 바꿔야함
constexpr int NPC_MOVE_TIME = 1000;
constexpr int NATURAL_HEALING_TIME = 5000;
constexpr int PLAYER_ATTACK_TIME = 1000;
constexpr int NPC_ATTACK_TIME = 1000;
constexpr int RESPAWN_TIME = 5000;	//30000
constexpr int STAT_DISPLAY_TIME = 1000;

constexpr int WIDE_ATTACK_DAMAGE = 20;
constexpr int FORWARD_ATTACK_DAMAGE = 50;

constexpr int EXP_INCREASE = 50;	//2로 바꿔야함

constexpr char CS_LOGIN = 0;
constexpr char CS_MOVE = 1;
constexpr char CS_CHAT = 2;
constexpr char CS_ATTACK = 3;
constexpr char CS_TELEPORT = 4;			// RANDOM한 위치로 Teleport, Stress Test할 때 Hot Spot현상을 피하기 위해 구현
constexpr char CS_LOGOUT = 5;			// 클라이언트에서 정상적으로 접속을 종료하는 패킷
constexpr char CS_DIRECTION = 6;

constexpr char SC_LOGIN_INFO = 0;
constexpr char SC_ADD_OBJECT = 1;
constexpr char SC_REMOVE_OBJECT = 2;
constexpr char SC_MOVE_OBJECT = 3;
constexpr char SC_CHAT = 4;
constexpr char SC_LOGIN_OK = 5;
constexpr char SC_LOGIN_FAIL = 6;
constexpr char SC_STAT_CHANGE = 7;
constexpr char SC_ATTACK = 8;
constexpr char SC_DIRECTION = 9;
constexpr char SC_RESPAWN = 10;


enum Key { KEY_UP_LEFT, KEY_UP, KEY_UP_RIGHT, KEY_LEFT, KEY_NONE, KEY_RIGHT, KEY_DOWN_LEFT, KEY_DOWN, KEY_DOWN_RIGHT };
enum Direction { DIR_UP, DIR_LEFT, DIR_DOWN, DIR_RIGHT };
enum ATTACK_TYPE { ATTACK_FORWARD, ATTACK_WIDE };
enum HIT_TYPE { HIT_TYPE_NONE, HIT_TYPE_HIT, HIT_TYPE_DEAD };

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

typedef struct two_doubles {
	double x;
	double y;
} TD;

typedef struct two_shorts {
	short x;
	short y;
} TS;

typedef struct two_ushorts {
	unsigned short x;
	unsigned short y;
} TUS;

typedef struct four_bools {
	bool up;
	bool down;
	bool left;
	bool right;
} FB;


#pragma pack (push, 1)
//CLIENT TO SERVER
struct CS_LOGIN_PACKET {
	unsigned short size = sizeof(CS_LOGIN_PACKET);
	char type = CS_LOGIN;
	
	char name[NAME_SIZE];
};

struct CS_MOVE_PACKET {
	unsigned short size = sizeof(CS_MOVE_PACKET);
	char type = CS_MOVE;
	
	char direction;
	unsigned move_time;
};

struct CS_CHAT_PACKET {
	unsigned short size = sizeof(CS_CHAT_PACKET);
	
	char type = CS_CHAT;
	char mess[CHAT_SIZE];
};

struct CS_TELEPORT_PACKET {
	unsigned short size = sizeof(CS_TELEPORT_PACKET);
	char type = CS_TELEPORT;
};

struct CS_LOGOUT_PACKET {
	unsigned short size = sizeof(CS_LOGOUT_PACKET);
	char type = CS_LOGOUT;
};

struct CS_ATTACK_PACKET {
	unsigned short size = sizeof(CS_ATTACK_PACKET);
	char type = CS_ATTACK;

	char attack_type;
	unsigned int time;
};

struct CS_DIRECTION_PACKET {
	unsigned short size = sizeof(CS_DIRECTION_PACKET);
	char type = CS_DIRECTION;

	char direction;
};

//SERVER TO CLIENT
struct SC_LOGIN_INFO_PACKET {
	unsigned short size = sizeof(SC_LOGIN_INFO_PACKET);
	char type = SC_LOGIN_INFO;
	
	int		id;
	int		hp;
	int		max_hp;
	int		exp;
	int		level;
	short	x, y;
};

struct SC_ADD_OBJECT_PACKET {
	unsigned short size = sizeof(SC_ADD_OBJECT_PACKET);
	char type = SC_ADD_OBJECT;

	int		id;
	short	x, y;
	char	name[NAME_SIZE];
};

struct SC_REMOVE_OBJECT_PACKET {
	unsigned short size = sizeof(SC_REMOVE_OBJECT_PACKET);
	char type = SC_REMOVE_OBJECT;

	int id;
};

struct SC_MOVE_OBJECT_PACKET {
	unsigned short size = sizeof(SC_MOVE_OBJECT_PACKET);
	char type = SC_MOVE_OBJECT;

	int id;
	short x, y;
	unsigned int time{};
};

struct SC_CHAT_PACKET {
	unsigned short size = sizeof(SC_CHAT_PACKET);
	char type = SC_CHAT;

	int	id;
	char mess[CHAT_SIZE];
};

struct SC_LOGIN_OK_PACKET {
	unsigned short size = sizeof(SC_LOGIN_OK_PACKET);
	char	type = SC_LOGIN_OK;
};

struct SC_LOGIN_FAIL_PACKET {
	unsigned short size = sizeof(SC_LOGIN_FAIL_PACKET);
	char	type = SC_LOGIN_FAIL;
};

struct SC_STAT_CHANGE_PACKET {
	unsigned short size = sizeof(SC_STAT_CHANGE_PACKET);
	char type = SC_STAT_CHANGE;
	
	int	hp;
	int	max_hp;
	int	exp;
	int	level;
};

struct SC_ATTACK_PACKET {
	unsigned short size = sizeof(SC_ATTACK_PACKET);
	char type = SC_ATTACK;

	int id;
	char hit_type;
	char attack_type;
};

struct SC_DIRECTION_PACKET {
	unsigned short size = sizeof(SC_DIRECTION_PACKET);
	char type = SC_DIRECTION;

	int id;
	char direction;
};

struct SC_RESPAWN_PACKET {
	unsigned short size = sizeof(SC_RESPAWN_PACKET);
	char type = SC_RESPAWN;

	int		id;
	int		hp;
	int		max_hp;
	int		exp;
	int		level;
	short	x, y;
};


#pragma pack (pop)