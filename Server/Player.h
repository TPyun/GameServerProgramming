#pragma once
#include"Global.h"

class Player
{
public:
	Player(TI, unsigned long long);
	~Player();
	void move(TI);
	void key_check();

	unsigned long long name{};
	KS key_input;
	TI position{ 0, 0 };
	TI size{ 40, 40 };

private:

};
