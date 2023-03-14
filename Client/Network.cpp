#include <iostream>
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
	
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 0), &WSAData);
	network->s_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, 0);
	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(network->SERVER_PORT);
	inet_pton(AF_INET, network->SERVER_ADDR, &server_addr.sin_addr);
	int retval = connect(network->s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	if (retval == SOCKET_ERROR) {
		cout << "Connection error" << endl;
		return 0;
	}
	cout << "Connected to server" << endl;

	//Recv data(player init pos)
	
	while (network->game->running()) {
		WSABUF send_buff;
		DWORD sent_byte;
		send_buff.buf = (char*)&network->game->key_input;
		send_buff.len = sizeof(network->game->key_input);
		WSASend(network->s_socket, &send_buff, 1, &sent_byte, 0, 0, 0);
		if (sent_byte == SOCKET_ERROR) {
			cout << "Send error" << endl;
			return 0;
		}
		network->game->key_input = { false, false, false, false };
		
		WSABUF recv_buff;
		DWORD recv_byte;
		DWORD recv_flag = 0;
		recv_buff.buf = (char*)&network->game->player->position;
		recv_buff.len = sizeof(network->game->player->position);
		WSARecv(network->s_socket, &recv_buff, 1, &recv_byte, &recv_flag, 0, 0);
		if (recv_byte == SOCKET_ERROR) {
			cout << "Recv error" << endl;
			return 0;
		}
	}
	return 0;
}
