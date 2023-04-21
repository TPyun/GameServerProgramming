#pragma once
#include "../Protocol.h"

class Player
{
public:
	KS key_input;
	TS position{ 0, 0 };
	TS size{ 40, 40 };
	
	Player();
	~Player();
	void move(TI);
	void key_check();

private:

};
