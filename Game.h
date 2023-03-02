#pragma once
#include <iostream>
#include <Windows.h>
#include "Global.h"
#include "Player.h"
#include "Additional\SDL2-2.24.0\include\SDL.h"
#include "Additional\SDL2_image-2.6.2\include\SDL_image.h"
#include "Additional\SDL2_mixer-2.6.2\include\SDL_mixer.h"
#include "Additional\SDL2_ttf-2.0.15\include\SDL_ttf.h"

using namespace std;

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

private:
	bool isRunning = true;
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Event event{};
	TI window_size{ WIDTH, HEIGHT };
	TI user_moniter{ GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
	
	Player* player = new Player(TI{ 450, 750 });
};
