#include <iostream>
#include <sstream>
#include "Network.h"

using namespace std;

Network::Network(Game* game_ptr)
{
	game = game_ptr;
	HANDLE h_thread = CreateThread(NULL, 0, process, this, 0, NULL);
}

Network::~Network()
{
	WSACleanup();
}

DWORD __stdcall process(LPVOID arg)
{
	Network* network = reinterpret_cast<Network*>(arg);
	
	for (;;) {
		while (!network->connected) {
			while (!network->game->try_connect) {
				Sleep(100);
			}
			network->game->try_connect = false;

			WSADATA WSAData;
			WSAStartup(MAKEWORD(2, 0), &WSAData);
			network->s_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, 0);
			SOCKADDR_IN server_addr;
			ZeroMemory(&server_addr, sizeof(server_addr));
			server_addr.sin_family = AF_INET;

			string str(network->game->Port);
			int port_num = 0;
			stringstream ssInt(str);
			ssInt >> port_num;

			server_addr.sin_port = htons(port_num);
			inet_pton(AF_INET, network->game->IPAdress, &server_addr.sin_addr);
			int retval = connect(network->s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
			if (retval == SOCKET_ERROR) {
				cout << "Connection error" << endl;
				continue;
			}
			cout << "Connected to server" << endl;

			//Recv data(player init pos)
			WSABUF recv_buff;
			DWORD recv_byte;
			DWORD recv_flag = 0;
			recv_buff.buf = (char*)&network->game->player->position;
			recv_buff.len = sizeof(network->game->player->position);
			WSARecv(network->s_socket, &recv_buff, 1, &recv_byte, &recv_flag, 0, 0);
			if (recv_byte == SOCKET_ERROR) {
				network->connected = false;
				cout << "Recv error" << endl;
				break;
			}

			network->connected = true;
			network->game->scene = 1;
		}

		while (network->game->get_running() && network->connected) {
			if (network->game->key_input.up || network->game->key_input.down || network->game->key_input.left || network->game->key_input.right) {
				WSABUF send_buff{};
				DWORD sent_byte{};
				send_buff.buf = (char*)&network->game->key_input;
				send_buff.len = sizeof(network->game->key_input);
				WSASend(network->s_socket, &send_buff, 1, &sent_byte, 0, 0, 0);
				if (sent_byte == SOCKET_ERROR || sent_byte == 0) {
					network->connected = false;
					cout << "Send error" << endl;
					network->game->scene = 0;
					break;
				}
				network->game->key_input = { false, false, false, false };

				WSABUF recv_buff{};
				DWORD recv_byte{};
				DWORD recv_flag = 0;
				recv_buff.buf = (char*)&network->game->player->position;
				recv_buff.len = sizeof(network->game->player->position);
				WSARecv(network->s_socket, &recv_buff, 1, &recv_byte, &recv_flag, 0, 0);
				if (recv_byte == SOCKET_ERROR || recv_byte == 0) {
					network->connected = false;
					cout << "Recv error" << endl;
					network->game->scene = 0;
					break;
				}
			}
			else {
				Sleep(10);
			}
		}
	}
}
