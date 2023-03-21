#pragma once
#include"Global.h"

class Player
{
public:
	Player(TI);
	~Player();

	TI position{ 0, 0 };
	TI size{ 40, 40 };
	int name{};
private:
	
};
