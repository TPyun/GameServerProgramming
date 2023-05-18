#pragma once
#include "../Protocol.h"
#include <chrono>

enum State { ST_IDLE, ST_MOVE, ST_RUN, ST_PUSH, ST_ATTACK, ST_HIT };
class Player
{
public:
	TI position{ -400, -400 };
	Direction direction = DIR_DOWN;
	State state = ST_IDLE;
	unsigned char sprite_iter{};
	int id = -1;
	char name[30]{};
	
	unsigned int moved_time{};
	unsigned int attack_time{};
	unsigned int chat_time{};
	std::string chat;

	Player();
	Player(TI);
	~Player();
	
private:
	
};
