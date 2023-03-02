/// ======================
///	WARNING!!!!!!!!!!!!!!!
///	YOU HAVE TO RUN AS X64
/// ======================

#include "Game.h"

int SDL_main(int argc, char* argv[])
{
	cout << "================" << endl;
	cout << "USE WASD TO MOVE" << endl;
	cout << "================" << endl;

	Game game;
	while (game.running()) {
		clock_t start = clock();
		game.handleEvents();
		game.clear();
		game.update();
		game.render();
		clock_t end = clock();
		double time = (double)(end - start) / CLOCKS_PER_SEC;
		cout << "FPS: " << 1 / time << endl;
	}
	return 0;
}

