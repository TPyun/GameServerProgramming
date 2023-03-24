/// ======================
///	WARNING!!!!!!!!!!!!!!!
///	YOU HAVE TO RUN AS X64
/// ======================
#include "Game.h"
#include "Network.h"
#pragma comment(lib, "WS2_32.LIB")
using namespace std;
int SDL_main(int argc, char* argv[])
{
	Game* game = new Game();
	HANDLE h_thread = CreateThread(NULL, 0, process, game, 0, NULL);
	
	while (game->get_running()) {
		game->clear();
		game->update();
		game->render();
	}
	
	return 0;
}
