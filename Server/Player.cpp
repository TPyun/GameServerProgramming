#include "Player.h"

using namespace std;

Player::Player(TI pos, int name) : position(pos), name(name)
{
	cout<< "New Player: " << name << " Location: " << pos.x << ", " << pos.y << endl;
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
	std::cout << "Player Pos : " << position.x << ", " << position.y << std::endl;
}

void Player::key_check()
{
	if (key_input.up){
		move(TI{ 0, -100 });
		key_input.up = false;
	}
	if (key_input.down){
		move(TI{ 0, 100 });
		key_input.down = false;
	}
	if (key_input.left){
		move(TI{ -100, 0 });
		key_input.left = false;
	}
	if (key_input.right){
		move(TI{ 100, 0 });
		key_input.right = false;
	}
}
