#pragma once
#include"Global.h"

class Player
{
public:
	Player(TI);
	~Player();
	
	void move(TI);
	
	TI position{ 0, 0 };
	TI size{ 40, 40 };
private:
	
};