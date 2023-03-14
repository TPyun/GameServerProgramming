#pragma once
#include <iostream>
#include <string>
#include "SDL.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include "SDL_ttf.h"
#include "Global.h"
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
	Player* player = new Player(TI{ 0, 0 });
	
	char IPAdress[100] = "";
	char Port[100] = "";
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
