#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <SFML/Audio.hpp>
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <stdlib.h>
#include <concurrent_unordered_map.h>
#include "Player.h"

enum SOUNDS { SOUND_YELL, SOUND_HIT, SOUND_ATTACK, SOUND_MOVE, SOUND_ENV, SOUND_TURN_ON };
class Game
{
public:
	Game();
	~Game();
	void update();
	void render();
	void clear();
	int real_fps{};
	int set_fps = 120;
	
	bool get_running() { return isRunning; }
	void initialize_main();
	void initialize_ingame();
	void play_sound(char sound);
	void stop_sound(char sound);

	//ingame scene
	KS key_input{ false, false, false, false };
	std::unordered_map <int, Player> players;
	std::mutex players_mtx;

	char ip_address[20] = "127.0.0.1";
	//char ip_address[20] = "192.168.0.8";
	char Port[20] = "9000";
	char Name[20] = "";
	std::string text_input;

	bool try_connect = false;
	bool connected = false;
	int my_id = -1;
	int ping = 0;
	char scene = 0;
	char chat_message[MAX_CHAT] = "";
	
	bool move_flag = false;
	bool direction_flag = false;
	bool attack_flag = false;
	bool chat_flag = false;
	
private:
	TI get_relative_location(TI);
	TI get_relative_location(TF);
	void draw_sfml_text(TI, char[], sf::Color, int);
	void draw_sfml_text_s(TI, std::string, sf::Color, int);
	void draw_sfml_rect(TI, TI, sf::Color, sf::Color);
	void draw_sprite(int, sf::Color, char);
	void timer();
	
	void draw_main();
	void draw_game();
	void draw_stat();
	void draw_information_mode();
	void draw_chat_mode();
	
	void main_handle_events();
	void game_handle_events();
	void chat_mode_handle_events();
	
	sf::RenderWindow* sfml_window;
	sf::Font sfml_font;
	sf::Event sfml_event{};
	sf::Text sfml_text;
	sf::Texture player_texture[6];
	sf::Sprite player_sprite[6];
	sf::SoundBuffer sound_buffer[20];
	sf::Sound sounds[20];

	TI user_moniter{ GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
	bool isRunning = true;
	bool information_mode = false;
	bool chat_mode = false;
	bool chat_start = false;
	
	int input_height = 130;
	bool input_warning = false;
};
