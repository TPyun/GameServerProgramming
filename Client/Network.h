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
	bool connected = false;

private:
};
