#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <SFML/Audio.hpp>
#include <chrono>
#include "../Protocol.h"

enum State { ST_IDLE, ST_MOVE, ST_RUN, ST_PUSH, ST_FORWARD_ATTACK, ST_WIDE_ATTACK, ST_HIT };
class Player
{
public:
	TD curr_position{ -500, -500 };
	TI arr_position{ -500, -500 };

	char direction = DIR_DOWN;
	State state = ST_IDLE;
	unsigned char sprite_iter{ (unsigned char)(rand() % 255) };
	unsigned int sprite_time{};
	
	int id = -1;
	char name[30]{};
	
	float curr_hp{};
	int hp{};
	int max_hp{};
	float curr_exp{};
	int exp{};
	int level{};
	
	unsigned int moved_time{};
	unsigned int forward_attack_time{};
	unsigned int wide_attack_time{};
	unsigned int chat_time{};

	std::string chat;

	Player();
	~Player();
	
	sf::SoundBuffer sound_buffer[20];
	sf::Sound sounds[20];
	void play_sound(char);
	void stop_sound(char);
private:
	
};
