#pragma once
#include "Game.h"

using namespace std;
const short SERVER_PORT = 9000;

int main()
{
	Game* game = new Game();

	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 0), &WSAData);
	SOCKET s_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, 0);
	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	listen(s_socket, SOMAXCONN);
	INT addr_size = sizeof(server_addr);
	
	cout << "Port: " << SERVER_PORT << endl;
	
	for (;;) {
		cout << "Waiting for Client" << endl;

		SOCKET c_socket = WSAAccept(s_socket, reinterpret_cast<sockaddr*>(&server_addr), &addr_size, 0, 0);
		cout << "Client connected" << endl;

		WSABUF send_buff;
		DWORD sent_byte;
		send_buff.buf = (char*)&game->player->position;
		send_buff.len = sizeof(game->player->position);
		WSASend(c_socket, &send_buff, 1, &sent_byte, 0, 0, 0);
		if (sent_byte == 0) {
			cout << "Client Disconnected" << endl;
			continue;
		}

		for (;;) {
			WSABUF recv_buff{};
			DWORD recv_byte{};
			DWORD recv_flag = 0;
			recv_buff.buf = (char*)&game->key_input;
			recv_buff.len = sizeof(game->key_input);
			WSARecv(c_socket, &recv_buff, 1, &recv_byte, &recv_flag, 0, 0);
			if (recv_byte == SOCKET_ERROR || recv_byte == 0) {
				cout << "Client Disconnected" << endl;
				break;
			}
			
			game->update();

			WSABUF send_buff;
			DWORD sent_byte;
			send_buff.buf = (char*)&game->player->position;
			send_buff.len = sizeof(game->player->position);
			WSASend(c_socket, &send_buff, 1, &sent_byte, 0, 0, 0);
			if (sent_byte == SOCKET_ERROR || sent_byte == 0) {
				cout << "Client Disconnected" << endl;
				break;
			}

			Sleep(10);
		}
	}
	WSACleanup();
	delete game;
}