#pragma once
#include "../Protocol.h"

enum Direction { UP, LEFT, DOWN, RIGHT };
enum State { IDLE, MOVE };
class Player
{
public:
	TI position{ -500, -500 };
	Direction direction = DOWN;
	State state = IDLE;
	unsigned char sprite_iter{};
	unsigned long long name{};

	Player();
	Player(TI);
	~Player();
	
private:
	
};
