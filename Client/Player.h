#pragma once
#include "../Protocol.h"

class Player
{
public:
	TI position{ -500, -500 };
	TI size{ 40, 40 };
	unsigned long long name{};

	Player();
	Player(TI);
	~Player();
	
private:
	
};
