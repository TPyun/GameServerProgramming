#include "Game.h"

Game::Game()
{
	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		//cout << "Could not initialize SDL!" << SDL_GetError() << endl;
	}
	if (TTF_Init() < 0) {
		//cout << "Could not initialize SDL!" << SDL_GetError() << endl;
	}
	//Create Window
	window = SDL_CreateWindow("", user_moniter.x / 2 - window_size.x / 2, user_moniter.y / 2 - window_size.y / 2, window_size.x, window_size.y, SDL_WINDOW_OPENGL);
	if (window == NULL) {
		//cout << "Could not create window!" << SDL_GetError() << endl;;
	}
	// Create renderer
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
	if (renderer == NULL) {
		//cout << "Could not create renderer!" << SDL_GetError() << endl;
	}
}

Game::~Game()
{
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void Game::handleEvents()
{
	SDL_PollEvent(&event);
	if (event.type == SDL_QUIT || event.key.keysym.sym == SDLK_ESCAPE) {
		isRunning = false;
	}
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_UP) {
			std::cout << "UP" << std::endl;
			key_input.up = true;
		}
		if (event.key.keysym.sym == SDLK_DOWN) {
			std::cout << "DOWN" << std::endl;
			key_input.down = true;
		}
		if (event.key.keysym.sym == SDLK_LEFT) {
			std::cout << "LEFT" << std::endl;
			key_input.left = true;
		}
		if (event.key.keysym.sym == SDLK_RIGHT) {
			std::cout << "RIGHT" << std::endl;
			key_input.right = true;
		}
	}
}

void Game::update()
{
	draw_game();
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

SDL_Rect Game::get_rect(TI pos, TI size)
{
	SDL_Rect rect{ pos.x - size.x / 2, pos.y - size.y / 2, size.x, size.y };
	return rect;
}
