/// ======================
///	WARNING!!!!!!!!!!!!!!!
///	YOU HAVE TO RUN AS X64
/// ======================
#include "Game.h"
#include "Network.h"

using namespace std;

int main()
{
	Game* game = new Game();
	HANDLE h_thread = CreateThread(NULL, 0, process, game, 0, NULL);
	int fps = 0;
	clock_t start = clock();
	while (game->get_running()) {
		
		game->clear();
		game->update();
		game->render();
		
		//check fps
		fps++;
		clock_t end = clock();
		if (end - start >= 1000) {
			//cout << "fps: " << game->real_fps << endl;
			game->real_fps = fps;
			fps = 0;
			start = clock();
		}
	}
}
