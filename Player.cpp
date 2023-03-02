#include "Player.h"

Player::Player(TI position)
{
	this->position = position;
}

Player::~Player()
{
}

void Player::move(TI direction)
{
	position.x += direction.x;
	position.y += direction.y;
}
