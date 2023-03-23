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

#pragma once
#include <iostream>
#include <sstream>
#include "Game.h"
using namespace std;

Game* game;
SOCKET server_socket;
WSAOVERLAPPED over;
WSABUF send_wsa_buffer;
WSABUF recv_wsa_buffer;
constexpr int BUF_SIZE = 1000;
char recv_buffer[BUF_SIZE];
bool connected = false;

void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags);

void send()
{
	Sleep(50);

	send_wsa_buffer.buf = (char*)&game->key_input;
	send_wsa_buffer.len = sizeof(game->key_input);

	memset(&over, 0, sizeof(over));
	
	int retval = WSASend(server_socket, &send_wsa_buffer, 1, 0, 0, &over, send_callback);
	cout << "WSASend retval: " << retval << endl;
	//Send Error Handling
	if (retval == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING){
		cout << "WSASend failed with error: " << WSAGetLastError() << endl;
		connected = false;
		return;
	}
}

void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED recv_over, DWORD flags)
{
	cout << "Recv: " << num_bytes << endl;
	
	char* packet = recv_buffer;
	
	while (packet < recv_buffer + num_bytes) {
		char packet_size = *packet;
		char client_id = *(packet + 1);

		memcpy(&game->players[client_id].position, (packet + 2), sizeof(TI));
		if (game->players[client_id].position.x == -1 && game->players[client_id].position.y == -1) {
			game->mtx.lock();
			game->players.erase(client_id);
			game->mtx.unlock();
		}
		packet = packet + packet_size;
	}
	send();
}

void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags)
{
	cout << "Send: " << num_bytes << endl;
	
	game->key_input = { false, false, false, false };
	
	recv_wsa_buffer = { BUF_SIZE, recv_buffer };

	DWORD recv_flag = 0;
	memset(over, 0, sizeof(*over));
	
	int retval = WSARecv(server_socket, &recv_wsa_buffer, 1, 0, &recv_flag, over, recv_callback);
	cout << "WSARecv retval: " << retval << endl;
	//Recv Error Handling
	if (retval == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
		cout << "WSARecv failed with error: " << WSAGetLastError() << endl;
		connected = false;
		return;
	}
}

DWORD __stdcall process(LPVOID arg)
{
	game = reinterpret_cast<Game*>(arg);

	for (;;) {
		while (!connected) {
			while (!game->try_connect) {
				Sleep(100);
			}
			game->try_connect = false;
			
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
				continue;
			}
			cout << "Connected to server" << endl;

			connected = true;
			game->scene = 1;

			send();
			while (game->get_running() && connected) {
				SleepEx(10, true);
			}
			
			game->scene = 0;
			closesocket(server_socket);
			WSACleanup();
		}
	}
	return 0;
}
