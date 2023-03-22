#pragma once
#include <string>
#include <unordered_map>
#include "Player.h"

class Game
{
public:
	Game();
	~Game();

	void handle_events();
	void update();
	void render();
	void clear();

	bool get_running() { return isRunning; }
	void draw_main();
	void draw_game();
	void draw_text(TI, char[], SDL_Color);
	SDL_Rect get_rect(TI, TI);

	//ingame scene
	KS key_input{ false, false, false, false };
	//Player* player = new Player(TI{ 0, 0 });
	std::unordered_map <int, Player> players;

	char ip_address[100] = "127.0.0.1";
	char Port[100] = "9000";
	bool try_connect = false;
	char scene{};

private:
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Event event{};
	TI window_size{ WIDTH, HEIGHT };
	TI user_moniter{ GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
	TTF_Font* font;
	bool isRunning = true;
	
	//main scene
	int input_height = 130;
	std::string text_input;
};
