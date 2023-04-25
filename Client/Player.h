#pragma once
#include "../Protocol.h"

class Player
{
public:
	TI position{ -500, -500 };
	TI size{ BLOCK_SIZE / 3 * 2, BLOCK_SIZE / 3 * 2 };
	unsigned long long name{};

	Player();
	Player(TI);
	~Player();
	
private:
	
};
