#include "Game.h"

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
	window = SDL_CreateWindow("", user_moniter.x / 2 - window_size.x / 2, user_moniter.y / 2 - window_size.y / 2, window_size.x, window_size.y, SDL_WINDOW_OPENGL);
	if (window == NULL) {
		cout << "Could not create window!" << SDL_GetError() << endl;;
	}
	// Create renderer
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
	if (renderer == NULL) {
		cout << "Could not create renderer!" << SDL_GetError() << endl;
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
		if (event.key.keysym.sym == SDLK_w) {
			//cout << "W" << endl;
			if (player->position.y - 100 > 0 && player->position.y< HEIGHT) {
				player->move(TI{ 0, -100 });
			}
		}
		if (event.key.keysym.sym == SDLK_s) {
			//cout << "S" << endl;
			if (player->position.y > 0 && player->position.y +100 < HEIGHT) {
				player->move(TI{ 0, 100 });
			}
		}
		if (event.key.keysym.sym == SDLK_a) {
			//cout << "A" << endl;
			if (player->position.x - 100 > 0 && player->position.x < WIDTH) {
				player->move(TI{ -100, 0 });
			}
		}
		if (event.key.keysym.sym == SDLK_d) {
			//cout << "D" << endl;
			if (player->position.x > 0 && player->position.x + 100 < WIDTH) {
				player->move(TI{ 100, 0 });
			}
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
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			SDL_Rect rect;
			rect.x = i * 100;
			rect.y = j * 100;
			rect.w = 100;
			rect.h = 100;
			if ((i + j) % 2 == 0) {
				SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
			}
			else {
				SDL_SetRenderDrawColor(renderer, 100, 50, 0, SDL_ALPHA_OPAQUE);
			}
			SDL_RenderFillRect(renderer, &rect);
		}
	}
	
	//draw player
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_Rect player_rect{ player->position.x - player->size.x/2, player->position.y - player->size.y/2, player->size.x, player->size.y};
	SDL_RenderFillRect(renderer, &player_rect);
}
