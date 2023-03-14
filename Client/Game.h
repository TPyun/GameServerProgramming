#pragma once
#include <iostream>
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

	void handleEvents();
	void update();
	void render();
	void clear();
	
	bool running() { return isRunning; }
	void draw_game();
	SDL_Rect get_rect(TI, TI);

	KS key_input{ false, false, false, false };
	Player* player = new Player(TI{ 450, 750 });
	
private:
	
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Event event{};
	TI window_size{ WIDTH, HEIGHT };
	TI user_moniter{ GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
	bool isRunning = true;
	
};
