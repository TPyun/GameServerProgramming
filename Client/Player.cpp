#pragma once
#include "Player.h"

Player::Player()
{
	
}

Player::~Player()
{
	
}

void Player::play_sound(char sound)
{
	sounds[sound].play();
}

void Player::stop_sound(char sound)
{
	sounds[sound].stop();
}
