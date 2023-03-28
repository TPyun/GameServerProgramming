#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
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
	void draw_main();
	void main_handle_events();
	void draw_game();
	void game_handle_events();

	//ingame scene
	KS key_input{ false, false, false, false };
	std::unordered_map <int, Player> players;
	std::mutex mtx;

	char ip_address[100] = "127.0.0.1";
	char Port[100] = "9000";
	bool try_connect = false;
	bool connected = false;
	char scene{};

	void draw_sfml_text(TI, char[], sf::Color);
	void draw_sfml_rect(TI, TI, sf::Color, sf::Color);
	TI sfml_get_corrected_position(TI, TI);
private:
	sf::RenderWindow* sfml_window;
	sf::Font sfml_font;
	sf::Event sfml_event{};
	sf::Text sfml_text;

	TI window_size{ WIDTH, HEIGHT };

	TI user_moniter{ GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
	bool isRunning = true;
	
	//main scene
	int input_height = 130;
	std::string text_input;
};
