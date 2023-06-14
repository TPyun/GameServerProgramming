#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <stdlib.h>
#include <concurrent_unordered_map.h>
#include "Player.h"
enum SOUNDS { SOUND_HIT, SOUND_ATTACK, SOUND_SWORD_HIT, SOUND_SWORD_ATTACK, SOUND_MOVE, SOUND_ENV, SOUND_TURN_ON , SOUND_TAB, SOUND_DEAD, SOUND_FIRE, SOUND_DOOM, SOUND_STAT, SOUND_DOWN, SOUND_ERROR, SOUND_BEAM};
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
	void play_sound(char, bool);
	void stop_sound(char);

	//ingame scene
	FB key_input{ false, false, false, false };
	std::unordered_map <int, Player> players;
	std::mutex players_mtx;

	char ip_address[30] = "127.0.0.1";
	//char ip_address[20] = "192.168.0.22";
	char Port[30] = "9000";
	char Name[30] = "";
	std::string text_input;

	bool try_connect = false;
	bool connected = false;
	int my_id = -1;
	int ping = 0;
	char scene = 0;
	char chat_message[CHAT_SIZE] = "";
	
	bool move_flag = false;
	bool direction_flag = false;
	bool forward_attack_flag = false;
	bool wide_attack_flag = false;
	bool chat_flag = false;

	bool connect_warning = false;
	bool id_warning = false;
	
	bool dead = true;
	int dead_time{};
	int left_dead_time{};

	int stat_chaged_time{};
	int left_stat_chaged_time{};

	int hp_change{};
	int max_hp_change{};
	int level_change{};
	int exp_change{};
	
	int attack_success_time{};
	int left_attack_success_time{};
	int attack_success_type{};

	
private:
	TI get_relative_location(TI);
	TI get_relative_location(TD);
	void draw_sfml_text(TI, char[], sf::Color, int);
	void draw_sfml_text_s(TI, std::string, sf::Color, int);
	void draw_sfml_rect(TI, TI, sf::Color, sf::Color, int);
	void draw_player_sprite(int, sf::Color, char);
	void draw_monster_sprite(int, sf::Color, char);
	void draw_obstacle(int);
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

	sf::Texture player_texture[7];
	sf::Sprite player_sprite[7];

	sf::Texture monster_texture[4];
	sf::Sprite monster_sprite[4];

	sf::Texture grass_texture;
	sf::Sprite grass_sprite;
	
	sf::Texture fire_texture;
	sf::Sprite fire_sprite;
	
	sf::Texture rock_texture;
	sf::Sprite rock_sprite;
	
	sf::SoundBuffer sound_buffer[100];
	sf::Sound sounds[100];
	
	TI user_moniter{ GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
	bool isRunning = true;
	bool information_mode = false;
	bool chat_mode = false;
	bool chat_start = false;
	bool distance_debug_mode = false;
	
	int input_height = 130;
	bool input_warning = false;
};
