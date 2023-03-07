/// ======================
///	WARNING!!!!!!!!!!!!!!!
///	YOU HAVE TO RUN AS X64
/// ======================

#include "Game.h"

int SDL_main(int argc, char* argv[])
{
	Game* game = new Game();
	while (game->running()) {
		game->handleEvents();
		game->clear();
		game->update();
		game->render();
	}
	return 0;
}
