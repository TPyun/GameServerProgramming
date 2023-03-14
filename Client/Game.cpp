#define _CRT_SECURE_NO_WARNINGS
#include "Game.h"

using namespace std;
Game::Game()
{
	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		cout << "Could not initialize SDL!" << SDL_GetError() << endl;
	}
	if (TTF_Init() < 0) {
		cout << "Could not initialize SDL!" << SDL_GetError() << endl;
	}
	//Create Window
	window = SDL_CreateWindow("Client", user_moniter.x / 2 - window_size.x / 2, user_moniter.y / 2 - window_size.y / 2, window_size.x, window_size.y, SDL_WINDOW_OPENGL);
	if (window == NULL) {
		cout << "Could not create window!" << SDL_GetError() << endl;;
	}
	// Create renderer
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
	if (renderer == NULL) {
		cout << "Could not create renderer!" << SDL_GetError() << endl;
	}
	// Load font
	font = TTF_OpenFont("arial.ttf", 17);
	if (!font) {
		printf("Could not open font! (%s)\n", TTF_GetError());
	}
	cout << "Game initialized!" << endl;
}

Game::~Game()
{
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void Game::handle_events()
{
	SDL_PollEvent(&event);
	if (event.type == SDL_QUIT || event.key.keysym.sym == SDLK_ESCAPE) {
		isRunning = false;
	}
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_UP) {
			//std::cout << "UP" << std::endl;
			key_input.up = true;
		}
		if (event.key.keysym.sym == SDLK_DOWN) {
			//std::cout << "DOWN" << std::endl;
			key_input.down = true;
		}
		if (event.key.keysym.sym == SDLK_LEFT) {
			//std::cout << "LEFT" << std::endl;
			key_input.left = true;
		}
		if (event.key.keysym.sym == SDLK_RIGHT) {
			//std::cout << "RIGHT" << std::endl;
			key_input.right = true;
		}
	}
}

void Game::update()
{
	if (scene == 0) {
		draw_main();
	}
	if (scene == 1) {
		handle_events();
		draw_game();
	}
}

void Game::render()
{
	SDL_RenderPresent(renderer);
}

void Game::clear()
{
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(renderer);
}

SDL_Rect Game::get_rect(TI pos, TI size)
{
	SDL_Rect rect{ pos.x - size.x / 2, pos.y - size.y / 2, size.x, size.y };
	return rect;
}

void Game::draw_main()
{
	SDL_PollEvent(&event);
	if (event.type == SDL_QUIT || event.key.keysym.sym == SDLK_ESCAPE) {
		isRunning = false;
	}
	
	//Press button to add text
	if (event.type == SDL_TEXTINPUT && strlen(text_input.c_str()) < 20) {
		text_input += event.text.text;
		if (input_height == 130) {
			strcpy(IPAdress, text_input.c_str());
		}
		else {
			strcpy(Port, text_input.c_str());
		}
	}
	//Press backspace to erase
	else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_BACKSPACE && text_input.size()) {
		text_input.pop_back();
		if (input_height == 130) {
			strcpy(IPAdress, text_input.c_str());
		}
		else {
			strcpy(Port, text_input.c_str());
		}
	}
	//Press Tab
	else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_TAB) {
		if (input_height == 330) {
			input_height = 130;
		}
		else {
			input_height += 100;
		}
		text_input = "";
	}
	else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN && input_height == 330) {
		try_connect = true;
	}
	SDL_Color color = { 200, 200, 200 };
	SDL_SetRenderDrawColor(renderer, 80, 80, 80, SDL_ALPHA_OPAQUE);
	SDL_Rect rect = { 99, input_height, 200, 20 };
	SDL_RenderFillRect(renderer, &rect);
	draw_text(TI{ 60, input_height }, (char*)"Tab", color);
	
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
	rect = { 99, 130, 200, 20 };
	SDL_RenderDrawRect(renderer, &rect);
	rect = { 99, 230, 200, 20 };
	SDL_RenderDrawRect(renderer, &rect);
	rect = { 99, 330, 200, 20 };
	SDL_RenderDrawRect(renderer, &rect);
	
	draw_text(TI{ 100, 100 }, (char*)"IP Address", color);
	draw_text(TI{ 100, 200 }, (char*)"Port", color);
	draw_text(TI{ 100, 300 }, (char*)"Play Game", color);
	draw_text(TI{ 100, 330 }, (char*)"Press Enter", color);


	draw_text(TI{ 100, 130 }, (char*)IPAdress, color);
	draw_text(TI{ 100, 230 }, (char*)Port, color);
}

void Game::draw_game()
{
	//Ã¼½º ÆÇ
	SDL_Rect rect;
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			rect.x = i * 100;
			rect.y = j * 100;
			rect.w = 100;
			rect.h = 100;
			if ((i + j) % 2 == 0) {
				SDL_SetRenderDrawColor(renderer, 200, 200, 200, SDL_ALPHA_OPAQUE);
			}
			else {
				SDL_SetRenderDrawColor(renderer, 100, 50, 0, SDL_ALPHA_OPAQUE);
			}
			SDL_RenderFillRect(renderer, &rect);
		}
	}
	
	//draw player
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_Rect player_rect = get_rect(player->position, player->size);
	SDL_RenderFillRect(renderer, &player_rect);
}

void Game::draw_text(TI pos, char text[], SDL_Color color)
{
	if (!font) {
		printf("Could not open font! (%s)\n", TTF_GetError());
		return;
	}
	SDL_Surface* surface = TTF_RenderText_Blended(font, text, color);
	if (!surface) {
		//cout << "no surface" << endl;
		return;
	}
	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

	SDL_Rect r = { pos.x, pos.y, surface->w, surface->h };
	SDL_RenderCopy(renderer, texture, NULL, &r);

	SDL_FreeSurface(surface);
	SDL_DestroyTexture(texture);
}