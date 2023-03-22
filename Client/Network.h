//#pragma once
//#include <thread>
//#include "Global.h"
//#include "Game.h"
//
//DWORD WINAPI process(LPVOID arg);
//
//class Network
//{
//public:
//	Network(Game*);
//	~Network();
//	
//	Game* game;
//	//SOCKET server_socket;
//	bool connected = false;
//
//private:
//};


#include <iostream>
#include <sstream>
#include "Game.h"
using namespace std;

WSAOVERLAPPED over;
WSABUF send_wsa_buffer;
WSABUF recv_wsa_buffer;
constexpr int BUF_SIZE = 512;
char recv_buffer[BUF_SIZE];
Game* game;
SOCKET server_socket;
bool connected = false;

void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags);

void send()
{
	Sleep(10);
	if (game->key_input.up || game->key_input.down || game->key_input.left || game->key_input.right)
		cout << game->key_input.up << " " << game->key_input.down << " " << game->key_input.left << " " << game->key_input.right << endl;
	
	send_wsa_buffer.buf = (char*)&game->key_input;
	send_wsa_buffer.len = sizeof(game->key_input);

	memset(&over, 0, sizeof(over));
	
	WSASend(server_socket, &send_wsa_buffer, 1, 0, 0, &over, send_callback);
}

void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED recv_over, DWORD flags)
{
	char* p = recv_buffer;
	
	while (p < recv_buffer + num_bytes) {
		char packet_size = *p;
		char client_id = *(p + 1);

		memcpy(&game->players[client_id].position, (p + 2), sizeof(game->players[client_id].position));
		cout << "ID: " << (int)client_id << " " << game->players[client_id].position.x << " " << game->players[client_id].position.y << endl;

		p = p + packet_size;
	}
	send();
}

void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags)
{
	game->key_input = { false, false, false, false };
	
	recv_wsa_buffer = { BUF_SIZE, recv_buffer };

	DWORD recv_flag = 0;
	memset(over, 0, sizeof(*over));
	WSARecv(server_socket, &recv_wsa_buffer, 1, 0, &recv_flag, over, recv_callback);
}

DWORD __stdcall process(LPVOID arg)
{
	Game* game_ptr = reinterpret_cast<Game*>(arg);
	game = game_ptr;

	for (;;) {
		while (!connected) {
			while (!game->try_connect) {
				Sleep(100);
			}

			WSADATA WSAData;
			WSAStartup(MAKEWORD(2, 0), &WSAData);
			server_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, 0);
			SOCKADDR_IN server_address;
			ZeroMemory(&server_address, sizeof(server_address));
			server_address.sin_family = AF_INET;

			string str(game->Port);
			int port_num = 0;
			stringstream ssInt(str);
			ssInt >> port_num;

			server_address.sin_port = htons(port_num);
			inet_pton(AF_INET, game->ip_address, &server_address.sin_addr);

			int retval = WSAConnect(server_socket, reinterpret_cast<sockaddr*>(&server_address), sizeof(server_address), 0, 0, 0, 0);
			if (retval == SOCKET_ERROR) {
				cout << "Connection error" << endl;
				return 0;
			}
			cout << "Connected to server" << endl;

			connected = true;
			game->scene = 1;

			send();

			while (game->get_running() && connected) {
				SleepEx(10, true);
			}
		}
		closesocket(server_socket);
		WSACleanup();
	}
	return 0;
}
