#pragma once
#include <iostream>
#include <sstream>
#include "Game.h"
using namespace std;
using namespace chrono;
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
void process_packet(char* packet);

void send(char* packet)
{
	if (!game->connected) return;
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
	if (!game->connected) return;
	//cout << "recv" << endl;

	recv_wsa_buffer.buf = recv_buffer + remain_data;
	recv_wsa_buffer.len = BUF_SIZE - remain_data;

	DWORD recv_flag = 0;
	int retval = WSARecv(server_socket, &recv_wsa_buffer, 1, 0, &recv_flag, &recv_over, recv_callback);
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
	//패킷 잘리면 이어 붙이기
	/*if (remain_data) {
		cout << "remain_data: " << remain_data << endl;
	}*/
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
}

void send_move_packet()
{
	CS_MOVE_PACKET move_packet;
	move_packet.ks = game->key_input;
	move_packet.time = static_cast<unsigned>(duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count());
	send((char*)&move_packet);
	ZeroMemory(&game->key_input, sizeof(KS));
	game->move_flag = false;
}

void send_direction_packet()
{
	CS_DIRECTION_PACKET direction_packet;
	direction_packet.direction = game->players[game->my_id].direction;
	send((char*)&direction_packet);
	game->direction_flag = false;
}

void send_attack_packet()
{
	CS_ATTACK_PACKET attack_packet;
	attack_packet.time = static_cast<unsigned>(duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count());
	send((char*)&attack_packet);
	game->attack_flag = false;
	
	game->players[game->my_id].sprite_iter = 0;
	game->players[game->my_id].attack_time = static_cast<unsigned>(duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count());
}

void send_chat_packet()
{
	CS_CHAT_PACKET chat_packet;
	memcpy(chat_packet.message, game->chat_message, MAX_CHAT);
	send((char*)&chat_packet);
	game->chat_flag = false;

	game->players[game->my_id].chat = game->chat_message;
	game->players[game->my_id].chat_time = static_cast<unsigned>(duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count());
	
	ZeroMemory(game->chat_message, sizeof(game->chat_message));
	game->text_input = "";
	
	//cout << "send chat" << endl;
}

void process_packet(char* packet)
{
	switch (packet[1]) {
	case P_SC_MOVE:
	{
		SC_MOVE_PACKET* recv_packet = reinterpret_cast<SC_MOVE_PACKET*>(packet);
		int client_id = recv_packet->client_id;
		game->players_mtx.lock();
		if(game->players[client_id].position.x < recv_packet->position.x){
			game->players[client_id].direction = DIR_RIGHT;
			game->players[client_id].state = ST_MOVE;
		}
		else if (game->players[client_id].position.x > recv_packet->position.x){
			game->players[client_id].direction = DIR_LEFT;
			game->players[client_id].state = ST_MOVE;
		}
		else if (game->players[client_id].position.y > recv_packet->position.y){
			game->players[client_id].direction = DIR_UP;
			game->players[client_id].state = ST_MOVE;
		}
		else if (game->players[client_id].position.y < recv_packet->position.y) {
			game->players[client_id].direction = DIR_DOWN;
			game->players[client_id].state = ST_MOVE;
		}

		game->players[client_id].position = recv_packet->position;
		game->players[client_id].id = client_id;
		game->players[client_id].moved_time = recv_packet->time;
		game->players_mtx.unlock();

		if (client_id == game->my_id)
			game->ping = duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count() - recv_packet->time;
		//cout << "client_id: " << client_id << " x: " << recv_packet->position.x << " y: " << recv_packet->position.y << endl;
		break;
	}
	case P_SC_DIRECTION:
	{
		SC_DIRECTION_PACKET* recv_packet = reinterpret_cast<SC_DIRECTION_PACKET*>(packet);
		int client_id = recv_packet->client_id;
		game->players_mtx.lock();
		game->players[client_id].direction = recv_packet->direction;
		game->players_mtx.unlock();
		//cout << "client_id: " << client_id << " direction: " << recv_packet->direction << endl;
		break;
	}
	case P_SC_ATTACK:
	{
		SC_ATTACK_PACKET* recv_packet = reinterpret_cast<SC_ATTACK_PACKET*>(packet);
		int client_id = recv_packet->client_id;

		game->players_mtx.lock();
		game->players[client_id].attack_time = duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
		game->players[client_id].sprite_iter = 0;
		game->players_mtx.unlock();
		//cout << recv_packet->client_id << " attack" << endl;
		break;
	}
	case P_SC_IN:
	{
		SC_IN_PACKET* recv_packet = reinterpret_cast<SC_IN_PACKET*>(packet);
		int client_id = recv_packet->client_id;
		
		game->players_mtx.lock();
		game->players[client_id].position = recv_packet->position;
		game->players[client_id].state = ST_IDLE;
		memcpy(game->players[client_id].name, recv_packet->name, 30);
		game->players_mtx.unlock();
		//cout << "IN client_id: " << client_id << " name: " << recv_packet->name << " " << game->players[client_id].position.x << " " << game->players[client_id].position.y << endl;
		break;
	}
	case P_SC_OUT:
	{
		SC_OUT_PACKET* recv_packet = reinterpret_cast<SC_OUT_PACKET*>(packet);
		int client_id = recv_packet->client_id;
		
		game->players_mtx.lock();
		int ret = game->players.erase(client_id);
		game->players_mtx.unlock();
		//cout << "client_id: " << client_id << " out" << endl;
		break;
	}
	case P_SC_CHAT:
	{
		SC_CHAT_PACKET* recv_packet = reinterpret_cast<SC_CHAT_PACKET*>(packet);
		int client_id = recv_packet->client_id;
		game->players[client_id].chat = recv_packet->message;
		game->players[client_id].chat_time = static_cast<unsigned>(duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count());
		//cout << "client_id: " << client_id << " chat: " << game->players[client_id].chat << endl;
		break;
	}
	case P_SC_LOGIN_INFO:
	{
		SC_LOGIN_INFO_PACKET* recv_packet = reinterpret_cast<SC_LOGIN_INFO_PACKET*>(packet);
		game->my_id = recv_packet->client_id;
		game->players_mtx.lock();
		game->players[game->my_id].position.x = recv_packet->position.x;
		game->players[game->my_id].position.y = recv_packet->position.y;
		game->players[game->my_id].id = game->my_id;
		memcpy(game->players[game->my_id].name, recv_packet->name, sizeof(recv_packet->name));
		game->players_mtx.unlock();
		
		cout << (char*)game->players[game->my_id].name << endl;
		//cout << "my id: " << game->my_id << endl;
		
		game->initialize_ingame();
		break;
	}
	default: cout << "Unknown Packet Type" << endl; break;
	}
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
		memcpy(login_packet.name, game->Name, sizeof(login_packet.name));
		send((char*)&login_packet);
		recv();
		while (game->get_running() && game->connected) {
			if (game->move_flag) {
				send_move_packet();
			}
			if (game->direction_flag) {
				send_direction_packet();
			}
			if (game->chat_flag) {
				send_chat_packet();
			}
			if (game->attack_flag) {
				send_attack_packet();
			}
			SleepEx(10, true);
		}
		
		game->players_mtx.lock();
		game->players.clear();
		game->players_mtx.unlock();
		game->scene = 0;
		closesocket(server_socket);
		WSACleanup();
	}
	return 0;
}
