#include "Player.h"

Player::Player(TI pos) : position(pos)
{
}

Player::~Player()
{
}

void Player::move(TI direction)
{
	if (position.x + direction.x < 0 || position.x + direction.x > HEIGHT || position.y + direction.y < 0 || position.y + direction.y > HEIGHT) {
		return;
	}
	position.x += direction.x;
	position.y += direction.y;
}