#pragma once
#include "../Protocol.h"

class Player
{
public:
	KS key_input;
	TI position{ 0, 0 };
	Direction direction = DIR_DOWN;
	TC ti_direction{ 0, 0 };
	char name[30]{};

	int	hp{ 100 };
	int	max_hp{ 100 };
	int	exp{};
	int	level{1};
	
	Player();
	~Player();
	void move(TC);
	void key_check();
	void increase_hp(int amount);
	bool decrease_hp(int amount);
	void increase_exp(int amount);
private:

};
