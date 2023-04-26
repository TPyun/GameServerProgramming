#pragma once
#include "../Protocol.h"

class Player
{
public:
	KS key_input;
	TI position{ 0, 0 };
	//TI size{ 40, 40 };
	
	Player();
	~Player();
	void move(TI);
	void key_check();

private:

};
