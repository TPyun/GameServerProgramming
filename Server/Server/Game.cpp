#include "Game.h"

Game::Game()
{
}

Game::~Game()
{
}

void Game::update()
{
	if (key_input.up)
	{
		player->move(TI{ 0, -100 });
	}
	if (key_input.down)
	{
		player->move(TI{ 0, 100 });
	}
	if (key_input.left)
	{
		player->move(TI{ -100, 0 });
	}
	if (key_input.right)
	{
		player->move(TI{ 100, 0 });
	}
	std::cout << "Player Pos : " << player->position.x << ", " << player->position.y << std::endl;

}
