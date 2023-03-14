#pragma once
#include <thread>
#include "Global.h"
#include "Game.h"

DWORD WINAPI process(LPVOID arg);

class Network
{
public:
	Network(Game*);
	~Network();
	
	Game* game;
	SOCKET s_socket;

	const char* SERVER_ADDR = "127.0.0.1";
	const short SERVER_PORT = 9000;
private:
	
};
