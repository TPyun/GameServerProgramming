#pragma once
#include "../Protocol.h"
#include <chrono>

enum State { ST_IDLE, ST_MOVE, ST_RUN, ST_PUSH, ST_ATTACK, ST_HIT };
class Player
{
public:
	TI position{ -500, -500 };
	Direction direction = DIR_DOWN;
	State state = ST_IDLE;
	unsigned char sprite_iter{ (unsigned char)(rand() % 255) };
	unsigned int sprite_time{};
	
	int id = -1;
	char name[30]{};
	
	int hp{};
	int max_hp{};
	int exp{};
	int level{};
	
	unsigned int moved_time{};
	unsigned int attack_time{};
	unsigned int chat_time{};
	std::string chat;

	Player();
	Player(TI);
	~Player();
	
private:
	
};
