#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include "Player.h"

class Game
{
public:
	Game();
	~Game();
	void update();
	void render();
	void clear();
	
	bool get_running() { return isRunning; }
	void initialize_ingame();

	//ingame scene
	KS key_input{ false, false, false, false };
	std::unordered_map <int, Player> players;
	std::mutex mtx;

	char ip_address[100] = "127.0.0.1";
	//char ip_address[100] = "192.168.0.8";
	char Port[100] = "9000";
	bool try_connect = false;
	bool connected = false;
	int my_id = -1;
	int ping = 0;
	bool move_flag = false;
	char scene = 0;
	
private:
	void draw_sfml_text(TI, char[], sf::Color, int);
	void draw_sfml_text_s(TI, std::string, sf::Color, int);
	void draw_sfml_rect(TI, TI, sf::Color, sf::Color);
	
	void draw_main();
	void main_handle_events();
	void draw_game();
	void draw_information();
	void game_handle_events();
	TI get_relative_location(TI, TI);
	
	sf::RenderWindow* sfml_window;
	sf::Font sfml_font;
	sf::Event sfml_event{};
	sf::Text sfml_text;

	TI window_size{ WIDTH, HEIGHT };

	TI user_moniter{ GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
	bool isRunning = true;
	bool information_mode = false;
	
	//main scene
	int input_height = 130;
	std::string text_input;
};
