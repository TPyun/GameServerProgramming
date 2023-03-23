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
const int BUFSIZE = 100;

void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED send_over, DWORD recv_flag);
void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED recv_over, DWORD recv_flag);
void delete_session(int);

class PACKET {
public:
	WSAOVERLAPPED send_over;	//클래스의 맨 첫번째에 있어야만 함. 이거를 가지고 클래스 포인터 위치 찾을거임
	int root_client_id;
	WSABUF send_wsa_buf;
	char send_data[BUFSIZE];
	
	PACKET(int client_id, int root_client_id, TI data) : root_client_id(root_client_id)
	{
		ZeroMemory(&send_over, sizeof(send_over));
		send_over.hEvent = reinterpret_cast<HANDLE>(client_id);
		
		char data_size = sizeof(TI);
		send_data[0] = data_size + 2;							// 1. size
		if (client_id == root_client_id) {						// 2. client id
			send_data[1] = 0;
		}
		else {
			send_data[1] = static_cast<char>(root_client_id);
		}
		memcpy(send_data + 2, &data, data_size);		// 3. real data

		send_wsa_buf.buf = send_data;
		send_wsa_buf.len = data_size + 2;
	}

	~PACKET() {}
};

class SESSION {
private:
	SOCKET socket;
	WSAOVERLAPPED recv_over;
	int client_id;
	
public:
	WSABUF recv_wsa_buff;
	Player player{ TI{ 450, 750 }, client_id};

	SESSION() {
		cout << "Unexpected Constructor Call Error!\n";
		exit(-1);
	}
	
	SESSION(int id, SOCKET s) : client_id(id), socket(s) {
		recv_wsa_buff.buf = (char*)&player.key_input;
		recv_wsa_buff.len = sizeof(player.key_input);
	}
	
	~SESSION() { closesocket(socket); }
	
	void do_recv() {
		DWORD recv_flag = 0;
		ZeroMemory(&recv_over, sizeof(recv_over));
		recv_over.hEvent = reinterpret_cast<HANDLE>(client_id);
		cout << "Recv client id " << client_id << endl;
		
		WSARecv(socket, &recv_wsa_buff, 1, 0, &recv_flag, &recv_over, recv_callback);
	}
	
	void do_send(int root_client_id, TI data) {
		cout << "DO SEND " << client_id << endl;
		PACKET* exp_over = new PACKET{ client_id, root_client_id, data };
		
		WSASend(socket, &exp_over->send_wsa_buf, 1, 0, 0, &exp_over->send_over, send_callback);
	}
};

unordered_map <int, SESSION> clients;

void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED send_over, DWORD recv_flag)
{
	PACKET* exp_over = reinterpret_cast<PACKET*>(send_over);	//PACKET 클래스의 첫번째 변수 주소를 가지고 PACKET 클래스 포인터를 찾음
	int client_id = reinterpret_cast<int>(exp_over->send_over.hEvent);
	if (err != 0) {
		cout << "send_callback Error: " << err << " Client ID: " << (int)send_over->hEvent << endl;
		delete_session(client_id);
	}
	
	cout << "send_callback: " << client_id << endl;
	delete exp_over;
}

void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED recv_over, DWORD recv_flag)	
{
	if (err != 0) {
		cout << "recv_callback Error: " << err << " Client_ID: " << (int)recv_over->hEvent << endl;
		int root_client_id = reinterpret_cast<int>(recv_over->hEvent);
		delete_session(root_client_id);
		return;
	}
	
	//recv하고나서 클라한테 받은 데이터 적용
	int root_client_id = reinterpret_cast<int>(recv_over->hEvent);
	cout << "recv_callback: " << root_client_id << endl;
	clients[root_client_id].player.key_check();

	//모든 클라한테 데이터 전송
	for (auto& client : clients) {
		cout << "SEND TO " << client.first << " FROM " << root_client_id << endl;
		client.second.do_send(root_client_id, clients[root_client_id].player.position);
	}
	
	clients[root_client_id].do_recv();
}

void delete_session(int client_id) 
{
	cout << "DELETE SESSION: " << client_id << endl;
	clients.erase(client_id);
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