#pragma once
#include "Player.h"

class Game
{
public:
	Game();
	~Game();
	void update();
	
	KS key_input;
	Player* player = new Player(TI{ 450, 750 });

private:

};