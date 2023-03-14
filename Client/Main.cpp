/// ======================
///	WARNING!!!!!!!!!!!!!!!
///	YOU HAVE TO RUN AS X64
/// ======================
#include "Network.h"

int SDL_main(int argc, char* argv[])
{
	Game* game = new Game();
	Network* network = new Network(game);
	
	while (game->running()) {
		game->handleEvents();
		game->clear();
		game->update();
		game->render();
	}
	
	return 0;
}
