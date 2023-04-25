#pragma once
#include <iostream>
#include <sstream>
#include "Game.h"
using namespace std;

Game* game;
SOCKET server_socket;
WSABUF send_wsa_buffer;
WSABUF recv_wsa_buffer;
WSAOVERLAPPED send_over;
WSAOVERLAPPED recv_over;

constexpr int BUF_SIZE = 4000;
char recv_buffer[BUF_SIZE];
int remain_data{};

void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags);
void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags);

void send(char* packet)
{
	if (!game->connected) return;
	Sleep(1);
	//cout << "send" << endl;

	send_wsa_buffer.buf = packet;
	send_wsa_buffer.len = packet[0];
	
	int retval = WSASend(server_socket, &send_wsa_buffer, 1, 0, 0, &send_over, send_callback);
	//Send Error Handling
	if (retval == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING){
		cout << "WSASend failed with error: " << WSAGetLastError() << endl;
		game->connected = false;
		return;
	}
}

void recv()
{
	recv_wsa_buffer.buf = recv_buffer + remain_data;
	recv_wsa_buffer.len = BUF_SIZE - remain_data;

	DWORD recv_flag = 0;

	//cout << "recv" << endl;
	int retval = WSARecv(server_socket, &recv_wsa_buffer, 1, 0, &recv_flag, &recv_over, recv_callback);
	//Recv Error Handling
	if (retval == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
		cout << "WSARecv failed with error: " << WSAGetLastError() << endl;
		game->connected = false;
		return;
	}
}

void process_packet(char* packet)
{
	switch (packet[1]) {
	case P_SC_MOVE:
	{
		SC_MOVE_PACKET* recv_packet = reinterpret_cast<SC_MOVE_PACKET*>(packet);
		int client_id = recv_packet->client_id;
		game->players[client_id].position.x = recv_packet->position.x;
		game->players[client_id].position.y = recv_packet->position.y;
		cout << "client_id: " << client_id << " x: " << recv_packet->position.x << " y: " << recv_packet->position.y << endl;
		break;
	}
	case P_SC_OUT:
	{
		SC_OUT_PACKET* recv_packet = reinterpret_cast<SC_OUT_PACKET*>(packet);
		int client_id = recv_packet->client_id;
		game->mtx.lock();
		game->players.erase(client_id);
		game->mtx.unlock();
		cout << "client_id: " << client_id << " out" << endl;
		break;
	}
	case P_SC_LOGIN:
	{
		SC_LOGIN_PACKET* recv_packet = reinterpret_cast<SC_LOGIN_PACKET*>(packet);
		game->my_id = recv_packet->client_id;
		game->players[game->my_id].position.x = recv_packet->position.x;
		game->players[game->my_id].position.y = recv_packet->position.y;
		//cout << "my id: " << game->my_id << endl;
		break;
	}
	default: cout << "Unknown Packet Type" << endl; break;
	}
}

void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags)
{
	if (err != 0) {
		game->connected = false;
		return;
	}
	//패킷 잘리면 이어 붙이기
	if (remain_data) {
		cout << "remain_data: " << remain_data << endl;
	}
	int data_to_proccess = num_bytes + remain_data;
	char* packet = recv_buffer;
	while (data_to_proccess > 0) {
		int packet_size = packet[0];
		if (packet_size <= data_to_proccess) {
			process_packet(packet);
			packet += packet_size;
			data_to_proccess -= packet_size;
		}
		else break;
	}
	remain_data = data_to_proccess;
	if (data_to_proccess > 0) {
		memcpy(recv_buffer, packet, data_to_proccess);
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
		
		game->mtx.lock();
		game->players.clear();
		game->mtx.unlock();
		game->scene = 0;
		closesocket(server_socket);
		WSACleanup();
	}
	return 0;
}
