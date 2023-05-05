#include "Player.h"

using namespace std;

Player::Player()
{
}

Player::~Player()
{
}

void Player::move(TI direction)
{
	if (position.x + direction.x < 0 || position.x + direction.x >= MAP_SIZE || position.y + direction.y < 0 || position.y + direction.y >= MAP_SIZE) {
		return;
	}
	position.x += direction.x;
	position.y += direction.y;
}

void Player::key_check()
{
	if (key_input.up){
		move(TI{ 0, -1 });
		key_input.up = false;
	}
	if (key_input.down){
		move(TI{ 0, 1 });
		key_input.down = false;
	}
	if (key_input.left){
		move(TI{ -1, 0 });
		key_input.left = false;
	}
	if (key_input.right){
		move(TI{ 1, 0 });
		key_input.right = false;
	}
}
