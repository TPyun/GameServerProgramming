#include "Player.h"

using namespace std;

Player::Player()
{
}

Player::~Player()
{
}

void Player::move(TC direction)
{
	if (position.x + direction.x < 0 || position.x + direction.x >= MAP_SIZE || position.y + direction.y < 0 || position.y + direction.y >= MAP_SIZE) {
		return;
	}
	position.x += direction.x;
	position.y += direction.y;
}

void Player::key_check()
{
	bool direction_changed = false;
	if (key_input.up){
		ti_direction = TC{ 0, -1 };
		move(ti_direction);
		direction = DIR_UP;
		key_input.up = false;
	}
	if (key_input.down){
		ti_direction = TC{ 0, 1 };
		move(ti_direction);
		direction = DIR_DOWN;
		key_input.down = false;
	}
	if (key_input.left){
		ti_direction = TC{ -1, 0 };
		move(ti_direction);
		direction = DIR_LEFT;
		key_input.left = false;
	}
	if (key_input.right){
		ti_direction = TC{ 1, 0 };
		move(ti_direction);
		direction = DIR_RIGHT;
		key_input.right = false;
	}
}

void Player::increase_hp(int amount)
{
	hp += amount;
	if (hp > max_hp) {
		hp = max_hp;
	}
}

bool Player::decrease_hp(int amount)
{
	bool dead{ false };
	hp -= amount;
	if (hp <= 0) {
		dead = true;
	}
	return dead;
}

void Player::increase_exp(int amount)
{
	if (exp >= level * 100) {
		exp -= level * 100;
		level++;
		max_hp += 10;
		hp = max_hp;
	}
	else {
		exp += amount;
	}
}
