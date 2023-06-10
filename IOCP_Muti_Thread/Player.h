#pragma once
#include "../Protocol.h"

enum PERSONALITY {
	PERSONALITY_NORMAL,
	PERSONALITY_AGGRESSIVE,
};

class Player
{
public:
	char key_input;
	TI position{ 0, 0 };
	char direction = DIR_DOWN;
	TC tc_direction{ 0, 0 };
	char name[20]{};
	int	hp{ 100 };
	int	max_hp{ 100 };
	int	exp{};
	int	level{1};
	PERSONALITY personality;
	
	Player();
	~Player();
	void move(TC);
	void key_check(bool do_move = true);
	void dir_check(bool do_move = true);
	void increase_hp(int amount);
	bool decrease_hp(int amount);
	void increase_exp(int amount);
	bool natural_healing();
	
private:

};
