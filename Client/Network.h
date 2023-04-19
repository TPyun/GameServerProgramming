#pragma once
#include <iostream>
#include <sstream>
#include "Game.h"
using namespace std;

Game* game;
SOCKET server_socket;
WSABUF send_wsa_buffer;
WSABUF recv_wsa_buffer;
WSAOVERLAPPED over;
constexpr int BUF_SIZE = 1000;
char recv_buffer[BUF_SIZE];

void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags);
void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags);

void send(char* packet)
{
	if (!game->connected) return;
	Sleep(1);
	cout << "send" << endl;
	
	/*CS_MOVE_PACKET packet;
	packet.ks = game->key_input;
	send_wsa_buffer.buf = (char*)&packet;
	send_wsa_buffer.len = sizeof(packet);*/

	send_wsa_buffer.buf = packet;
	send_wsa_buffer.len = packet[0];
	
	int retval = WSASend(server_socket, &send_wsa_buffer, 1, 0, 0, &over, send_callback);
	//Send Error Handling
	if (retval == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING){
		cout << "WSASend failed with error: " << WSAGetLastError() << endl;
		game->connected = false;
		return;
	}
}

void recv()
{
	recv_wsa_buffer.buf = recv_buffer;
	recv_wsa_buffer.len = BUF_SIZE;

	DWORD recv_flag = 0;

	cout << "recv" << endl;
	int retval = WSARecv(server_socket, &recv_wsa_buffer, 1, 0, &recv_flag, &over, recv_callback);
	//Recv Error Handling
	if (retval == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
		cout << "WSARecv failed with error: " << WSAGetLastError() << endl;
		game->connected = false;
		return;
	}
}

void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags)
{
	if (err != 0) {
		game->connected = false;
		return;
	}
	
	char* packet = recv_buffer;
	while (packet < recv_buffer + num_bytes) {		//패킷 까기
		SC_MOVE_PACKET* recv_packet = reinterpret_cast<SC_MOVE_PACKET*>(packet);
		int client_id = recv_packet->client_id;
		game->players[client_id].position = recv_packet->position;
		unsigned char packet_size = recv_packet->size;
		TI player_out_location{ -1, -1 };
		if (!memcmp(&game->players[client_id].position, &player_out_location, sizeof(TI))) {
			game->mtx.lock();
			game->players.erase(client_id);
			game->mtx.unlock();
		}
		packet += packet_size;
	}
	recv();
}

void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags)
{
	if (err != 0) {
		game->connected = false;
		return;
	}
	
	//cout << "Send: " << num_bytes << endl;
	ZeroMemory(&game->key_input, sizeof(KS));		//전송 후 키입력 초기화
}

DWORD __stdcall process(LPVOID arg)
{
	game = reinterpret_cast<Game*>(arg);

	for (;;) {
		while (!game->try_connect) {
			Sleep(10);
		}
		game->try_connect = false;
		game->players.clear();

		WSADATA WSAData;
		WSAStartup(MAKEWORD(2, 0), &WSAData);
		server_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
		SOCKADDR_IN server_address;
		ZeroMemory(&server_address, sizeof(server_address));
		server_address.sin_family = AF_INET;

		string str(game->Port);
		int port_num{};
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
		game->connected = true;

		CS_LOGIN_PACKET login_packet;
		send((char*)&login_packet);
		recv();
		game->scene = 1;
		while (game->get_running() && game->connected) {
			if (game->key_input.up || game->key_input.down || game->key_input.left || game->key_input.right) {
				CS_MOVE_PACKET move_packet;
				move_packet.ks = game->key_input;
				send((char*)&move_packet);
			}
			SleepEx(10, true);
		}
		
		game->players.clear();
		game->scene = 0;
		closesocket(server_socket);
		WSACleanup();
	}
	return 0;
}
