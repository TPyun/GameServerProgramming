#include "Player.h"

Player::Player()
{
}

Player::~Player()
{
}

void Player::move(TC direction)
{
	if (position.x + direction.x < 0 || position.x + direction.x >= W_WIDTH || position.y + direction.y < 0 || position.y + direction.y >= W_WIDTH) {
		return;
	}
	position.x += direction.x;
	position.y += direction.y;
}

void Player::key_check()
{
	switch (key_input) {
	case KEY_UP:
		tc_direction = { 0, -1 };
		direction = DIR_UP;
		break;
	case KEY_DOWN:
		tc_direction = { 0, 1 };
		direction = DIR_DOWN;
		break;
	case KEY_LEFT:
		tc_direction = { -1, 0 };
		direction = DIR_LEFT;
		break;
	case KEY_RIGHT:
		tc_direction = { 1, 0 };
		direction = DIR_RIGHT;
		break;
	case KEY_UP_LEFT:
		tc_direction = { -1, -1 };
		direction = DIR_LEFT;
		break;
	case KEY_UP_RIGHT:
		tc_direction = { 1, -1 };
		direction = DIR_RIGHT;
		break;
	case KEY_DOWN_LEFT:
		tc_direction = { -1, 1 };
		direction = DIR_LEFT;
		break;
	case KEY_DOWN_RIGHT:
		tc_direction = { 1, 1 };
		direction = DIR_RIGHT;
		break;
	}
	move(tc_direction);
	key_input = KEY_NONE;
}

void Player::dir_check()
{
	switch (tc_direction.y) {
	case -1:
		direction = DIR_UP;
		break;
	case 1:
		direction = DIR_DOWN;
		break;
	}
	switch (tc_direction.x) {
	case -1:
		direction = DIR_LEFT;
		break;
	case 1:
		direction = DIR_RIGHT;
		break;
	}
	move(tc_direction);
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
