#pragma once
#include"Global.h"

class Player
{
public:
	int name{};
	KS key_input;
	TI position{ 0, 0 };
	TI size{ 40, 40 };
	
	Player(TI, int);
	~Player();
	void move(TI);
	void key_check();

private:

};
