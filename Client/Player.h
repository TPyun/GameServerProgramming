#pragma once
#include"Global.h"

class Player
{
public:
	Player();
	Player(TI);
	~Player();
	
	unsigned long long name{};
	TI position{ 0, 0 };
	TI size{ 40, 40 };
	
private:
	
};
