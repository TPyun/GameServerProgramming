//#pragma once
//#include "Game.h"
//
//using namespace std;
//const short SERVER_PORT = 9000;
//
//int main()
//{
//	Game* game = new Game();
//
//	WSADATA WSAData;
//	WSAStartup(MAKEWORD(2, 0), &WSAData);
//	SOCKET s_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, 0);
//	SOCKADDR_IN server_addr;
//	ZeroMemory(&server_addr, sizeof(server_addr));
//	server_addr.sin_family = AF_INET;
//	server_addr.sin_port = htons(SERVER_PORT);
//	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
//	bind(s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
//	listen(s_socket, SOMAXCONN);
//	INT addr_size = sizeof(server_addr);
//	
//	cout << "Port: " << SERVER_PORT << endl; 
//	
//	for (;;) {
//		cout << "Waiting for Client" << endl;
//
//		SOCKET c_socket = WSAAccept(s_socket, reinterpret_cast<sockaddr*>(&server_addr), &addr_size, 0, 0);
//		cout << "Client connected" << endl;
//
//		WSABUF send_buff;
//		DWORD sent_byte;
//		send_buff.buf = (char*)&game->player->position;
//		send_buff.len = sizeof(game->player->position);
//		WSASend(c_socket, &send_buff, 1, &sent_byte, 0, 0, 0);
//		
//		if (sent_byte == 0) {
//			cout << "Client Disconnected" << endl;
//			continue;
//		}
//
//		for (;;) {
//			WSABUF recv_buff{};
//			DWORD recv_byte{};
//			DWORD recv_flag = 0;
//			recv_buff.buf = (char*)&game->key_input;
//			recv_buff.len = sizeof(game->key_input);
//			WSARecv(c_socket, &recv_buff, 1, &recv_byte, &recv_flag, 0, 0);
//			
//			if (recv_byte == SOCKET_ERROR || recv_byte == 0) {
//				cout << "Client Disconnected" << endl;
//				break;
//			}
//			
//			game->update();
//
//			WSABUF send_buff;
//			DWORD sent_byte;
//			send_buff.buf = (char*)&game->player->position;
//			send_buff.len = sizeof(game->player->position);
//			WSASend(c_socket, &send_buff, 1, &sent_byte, 0, 0, 0);
//			
//			if (sent_byte == SOCKET_ERROR || sent_byte == 0) {
//				cout << "Client Disconnected" << endl;
//				break;
//			}
//
//			Sleep(10);
//		}
//	}
//	WSACleanup();
//	delete game;
//}

#include <iostream>
#include <unordered_map>
#include "Player.h"
#pragma comment (lib, "WS2_32.LIB")

using namespace std;

const short SERVER_PORT = 9000;
const int BUFSIZE = 256;

void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED send_over, DWORD recv_flag);
void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED recv_over, DWORD recv_flag);

class EXP_OVER {
public:
	WSAOVERLAPPED send_over;
	unsigned long long client_id;
	WSABUF wsa_buf;
	char send_data[BUFSIZE];
	
public:
	EXP_OVER(char num_bytes, unsigned long long client_id, TI data) : client_id(client_id)
	{
		ZeroMemory(&send_over, sizeof(send_over));
		//send_over.hEvent = reinterpret_cast<HANDLE>(client_id);

		send_data[0] = num_bytes + 2;							// 1. size
		send_data[1] = static_cast<char>(client_id);			// 2. client id
		memcpy(send_data + 2, &data, num_bytes);	// 3. real data

		wsa_buf.buf = send_data;
		wsa_buf.len = num_bytes + 2;
	}

	~EXP_OVER() {}
};

class SESSION {
private:
	SOCKET socket;
	WSAOVERLAPPED recv_over;
	unsigned long long client_id;
	
public:
	//WSABUF send_wsa_buff;
	WSABUF recv_wsa_buff;
	Player player{ TI{ 450, 750 }, client_id};

	SESSION() {
		cout << "Unexpected Constructor Call Error!\n";
		exit(-1);
	}
	
	SESSION(int id, SOCKET s) : client_id(id), socket(s) {
		recv_wsa_buff.buf = (char*)&player.key_input;
		recv_wsa_buff.len = sizeof(player.key_input);

		//send_wsa_buff.buf = (char*)&player.position;
		//send_wsa_buff.len = sizeof(player.position);
	}
	
	~SESSION() { closesocket(socket); }
	
	int send_i{};
	int recv_i{};
	
	void do_recv() {
		recv_i++;
		cout << "recv: " << client_id << " " << recv_i << endl;

		DWORD recv_flag = 0;
		ZeroMemory(&recv_over, sizeof(recv_over));
		recv_over.hEvent = reinterpret_cast<HANDLE>(client_id);
		
		WSARecv(socket, &recv_wsa_buff, 1, 0, &recv_flag, &recv_over, recv_callback);
	}
	
	void do_send() {
		send_i++;
		cout << "send: " << client_id << " " << send_i << endl;
		
		EXP_OVER* send_over = new EXP_OVER(sizeof(player.position), client_id, player.position);
		cout << player.position.x << " " << player.position.y << endl;
		cout << "Size " << send_over->wsa_buf.len << endl;
		WSASend(socket, &send_over->wsa_buf, 1, 0, 0, &send_over->send_over, send_callback);
	}
};

unordered_map <unsigned long long, SESSION> clients;

void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED send_over, DWORD recv_flag)
{
	//unsigned long long client_id = reinterpret_cast<unsigned long long>(send_over->hEvent);
	delete reinterpret_cast<EXP_OVER*>(send_over);
}

void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED recv_over, DWORD recv_flag)	
{
	//recv하고나서 클라한테 받은 데이터 적용
	unsigned long long client_id = reinterpret_cast<unsigned long long>(recv_over->hEvent);
	
	//모든 클라한테 데이터 전송
	for (auto& client : clients) {
		//cout << client.second.player.key_input.up << " " << client.second.player.key_input.down << " " << client.second.player.key_input.left << " " << client.second.player.key_input.right << endl;
		client.second.player.key_check();
		client.second.do_send();
	}
	/*clients[client_id].player.key_check();
	clients[client_id].do_send();*/

	clients[client_id].do_recv();
}

int main()
{
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	SOCKET server_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(server_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	listen(server_socket, SOMAXCONN);
	INT addr_size = sizeof(server_addr);
	
	cout << "Port: " << SERVER_PORT << endl;
	
	for (int i = 1; ; ++i) {
		SOCKET client_socket = WSAAccept(server_socket, reinterpret_cast<sockaddr*>(&server_addr), &addr_size, 0, 0);
		clients.try_emplace(i, i, client_socket);
		clients[i].do_recv();
		cout << "Client added" << endl;
	}
	
	clients.clear();
	closesocket(server_socket);
	WSACleanup();
}