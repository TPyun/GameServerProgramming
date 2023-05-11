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
	
	int frame = 0;
	clock_t start = clock();
	while (game->get_running()) {
		
		game->clear();
		game->update();
		game->render();
		
		//check fps
		/*frame++;
		clock_t end = clock();
		if (end - start >= 1000) {
			cout << "fps: " << frame << endl;
			frame = 0;
			start = clock();
		}*/
	}
	
	return 0;
}
