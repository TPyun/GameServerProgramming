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
if (retval == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
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
	unsigned int current_time = static_cast<unsigned>(duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count());
	if (game->players[game->my_id].moved_time + PLAYER_MOVE_TIME > current_time) {
		//cout << game->players[game->my_id].moved_time + PLAYER_MOVE_TIME << " " << current_time << endl;
		return;
	}

	CS_MOVE_PACKET move_packet;
	bool moved = false;
	if (!game->key_input.left || !game->key_input.right) {
		if (game->key_input.left) {
			move_packet.direction = KEY_LEFT;
		}
		if (game->key_input.right) {
			move_packet.direction = KEY_RIGHT;
		}
		moved = true;
	}
	if (!game->key_input.up || !game->key_input.down) {
		if (game->key_input.up) {
			move_packet.direction = KEY_UP;
			if (game->key_input.left)
				move_packet.direction = KEY_UP_LEFT;
			else if (game->key_input.right)
				move_packet.direction = KEY_UP_RIGHT;
		}
		if (game->key_input.down) {
			move_packet.direction = KEY_DOWN;
			if (game->key_input.left)
				move_packet.direction = KEY_DOWN_LEFT;
			else if (game->key_input.right)
				move_packet.direction = KEY_DOWN_RIGHT;
		}
		moved = true;
	}
	if (moved) {
		move_packet.move_time = static_cast<unsigned>(duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count());
		send((char*)&move_packet);
		//cout << game->key_input.up << ", " << game->key_input.down << ", " << game->key_input.left << ", " << game->key_input.right << endl;
	}
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
}

void send_chat_packet()
{
	CS_CHAT_PACKET chat_packet;
	memcpy(chat_packet.mess, game->chat_message, CHAT_SIZE);
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
	switch (packet[2]) {
	case SC_MOVE_OBJECT:
	{
		SC_MOVE_OBJECT_PACKET* recv_packet = reinterpret_cast<SC_MOVE_OBJECT_PACKET*>(packet);
		int client_id = recv_packet->id;
		
		game->players_mtx.lock();
		if(game->players[client_id].arr_position.x < recv_packet->x){
			game->players[client_id].direction = DIR_RIGHT;
		}
		else if (game->players[client_id].arr_position.x > recv_packet->x){
			game->players[client_id].direction = DIR_LEFT;
		}
		else if (game->players[client_id].arr_position.y > recv_packet->y){
			game->players[client_id].direction = DIR_UP;
		}
		else if (game->players[client_id].arr_position.y < recv_packet->y) {
			game->players[client_id].direction = DIR_DOWN;
		}
		
		game->players[client_id].state = ST_MOVE;
		game->players[client_id].id = client_id;
		game->players[client_id].arr_position.x = recv_packet->x;
		game->players[client_id].arr_position.y = recv_packet->y;
		game->players[client_id].moved_time = recv_packet->time;
		game->players_mtx.unlock();

		if (client_id == game->my_id) {
			game->ping = duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count() - recv_packet->time;
			//cout << "client_id: " << client_id << " x: " << recv_packet->x << " y: " << recv_packet->y << endl;
		}
	}
	break;
	case SC_DIRECTION:
	{
		SC_DIRECTION_PACKET* recv_packet = reinterpret_cast<SC_DIRECTION_PACKET*>(packet);
		int client_id = recv_packet->id;
		
		game->players_mtx.lock();
		game->players[client_id].direction = recv_packet->direction;
		game->players_mtx.unlock();
		//cout << "client_id: " << client_id << " direction: " << recv_packet->direction << endl;
	}
	break;
	case SC_ATTACK:
	{
		SC_ATTACK_PACKET* recv_packet = reinterpret_cast<SC_ATTACK_PACKET*>(packet);
		int client_id = recv_packet->id;

		game->players_mtx.lock();
		game->players[client_id].attack_time = duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
		game->players[client_id].sprite_iter = 0;
		game->players_mtx.unlock();
		
		if (recv_packet->hit) {
			game->play_sound(SOUND_SWORD_HIT, false);
			
			if (recv_packet->dead)
				game->play_sound(SOUND_DEAD, false);
		}
		else {
			game->play_sound(SOUND_SWORD_ATTACK, false);
		}
		//cout << recv_packet->id << " attack" << endl;
	}
	break;
	case SC_ADD_OBJECT:
	{
		SC_ADD_OBJECT_PACKET* recv_packet = reinterpret_cast<SC_ADD_OBJECT_PACKET*>(packet);
		int client_id = recv_packet->id;
		
		game->players_mtx.lock();
		game->players[client_id].curr_position.x = recv_packet->x;
		game->players[client_id].curr_position.y = recv_packet->y;
		game->players[client_id].arr_position.x = recv_packet->x;
		game->players[client_id].arr_position.y = recv_packet->y;

		game->players[client_id].state = ST_IDLE;
		
		memcpy(game->players[client_id].name, recv_packet->name, 30);
		game->players_mtx.unlock();
		//cout << "IN client_id: " << client_id << " name: " << recv_packet->name << " " << game->players[client_id].arr_position.x << " " << game->players[client_id].arr_position.y << endl;
	}
	break;
	case SC_REMOVE_OBJECT:
	{
		SC_REMOVE_OBJECT_PACKET* recv_packet = reinterpret_cast<SC_REMOVE_OBJECT_PACKET*>(packet);
		int client_id = recv_packet->id;
		
		game->players_mtx.lock();
		int ret = game->players.erase(client_id);
		game->players_mtx.unlock();
		//cout << "client_id: " << client_id << " out" << endl;
	}
	break;
	case SC_STAT_CHANGE:
	{
		SC_STAT_CHANGE_PACKET* recv_packet = reinterpret_cast<SC_STAT_CHANGE_PACKET*>(packet);
		game->players[game->my_id].hp = recv_packet->hp;
		game->players[game->my_id].max_hp = recv_packet->max_hp;
		game->players[game->my_id].level = recv_packet->level;
		game->players[game->my_id].exp = recv_packet->exp;

		//cout << "hp: " << recv_packet->hp << " max_hp: " << recv_packet->max_hp << " level: " << recv_packet->level << " exp: " << recv_packet->exp << endl;
	}
	break;
	case SC_CHAT:
	{
		SC_CHAT_PACKET* recv_packet = reinterpret_cast<SC_CHAT_PACKET*>(packet);
		int client_id = recv_packet->id;
		game->players[client_id].chat = recv_packet->mess;
		game->players[client_id].chat_time = static_cast<unsigned>(duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count());
		//cout << "client_id: " << client_id << " chat: " << game->players[client_id].chat << endl;
	}
	break;
	case SC_LOGIN_INFO:
	{
		SC_LOGIN_INFO_PACKET* recv_packet = reinterpret_cast<SC_LOGIN_INFO_PACKET*>(packet);
		game->my_id = recv_packet->id;
		game->players_mtx.lock();
		game->players[game->my_id].curr_position.x = recv_packet->x;
		game->players[game->my_id].curr_position.y = recv_packet->y;
		game->players[game->my_id].arr_position.x = recv_packet->x;
		game->players[game->my_id].arr_position.y = recv_packet->y;

		game->players[game->my_id].id = game->my_id;
		memcpy(game->players[game->my_id].name, game->Name, sizeof(game->Name));
		game->players[game->my_id].hp = recv_packet->hp;
		game->players[game->my_id].max_hp = recv_packet->max_hp;
		game->players[game->my_id].level = recv_packet->level;
		game->players[game->my_id].exp = recv_packet->exp;
		game->players_mtx.unlock();
		
		//cout << "My Name: " << (char*)game->players[game->my_id].name << " My ID: " << game->my_id << endl;
		
		game->initialize_ingame();
	}
	break;
	case SC_LOGIN_FAIL:
	{
		SC_LOGIN_FAIL_PACKET* recv_packet = reinterpret_cast<SC_LOGIN_FAIL_PACKET*>(packet);
		//game->login_fail = true;
	}
	break;
	case SC_LOGIN_OK:
	{
		SC_LOGIN_OK_PACKET* recv_packet = reinterpret_cast<SC_LOGIN_OK_PACKET*>(packet);
		//game->login_ok = true;
	}
	break;
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
			game->connect_warning = true;
			continue;
		}
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
		
		game->initialize_main();
		closesocket(server_socket);
		WSACleanup();
	}
	return 0;
}
