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
#include "Game.h"
#include <unordered_map>
using namespace std;
#pragma comment (lib, "WS2_32.LIB")

const short SERVER_PORT = 9000;
const int BUFSIZE = 256;
Game game;

void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED send_over, DWORD recv_flag);
void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED recv_over, DWORD recv_flag);

//class EXP_OVER {
//public:
//	WSAOVERLAPPED wsa_over;
//	unsigned long long sender_id;
//	//char send_packet[BUFSIZE];
//	WSABUF send_wsa_buff;
//
//public:
//	EXP_OVER(unsigned long long id, char data_size, const char* data) : sender_id(id)
//	{
//		ZeroMemory(&wsa_over, sizeof(wsa_over));
//		//send_packet[0] = data_size + 2; //버퍼의 첫번째 바이트는 패킷의 크기
//		//send_packet[1] = static_cast<unsigned long long>(id);	//버퍼의 두번째 바이트는 클라이언트 아이디
//		//memcpy(send_packet + 2, data, data_size);	//패킷에 데이터 복사
//		//
//		//send_wsa_buff.buf = send_packet;
//		//send_wsa_buff.len = data_size + 2;	
//		send_wsa_buff.buf = (char*)&game.player->position;
//		send_wsa_buff.len = sizeof(game.player->position);
//	}
//	
//	~EXP_OVER() {}
//};

class SESSION {
private:
	SOCKET socket;
	WSAOVERLAPPED recv_over;
	unsigned long long id;
	
public:
	WSABUF recv_wsa_buff;
	//char recv_buffer[BUFSIZE];
	
	SESSION() {
		cout << "Unexpected Constructor Call Error!\n";
		exit(-1);
	}
	
	SESSION(int id, SOCKET s) : id(id), socket(s) {
		/*recv_wsa_buff.buf = recv_buffer; 
		recv_wsa_buff.len = BUFSIZE;*/
		recv_wsa_buff.buf = (char*)&game.key_input;
		recv_wsa_buff.len = sizeof(game.key_input);
	}
	
	~SESSION() { closesocket(socket); }
	
	void do_recv() {
		DWORD recv_flag = 0;
		ZeroMemory(&recv_over, sizeof(recv_over));
		recv_over.hEvent = reinterpret_cast<HANDLE>(id);
		
		WSARecv(socket, &recv_wsa_buff, 1, 0, &recv_flag, &recv_over, recv_callback);
	}
	
	void do_send(unsigned long long sender_id, int num_bytes, WSABUF buff) {
		//EXP_OVER* send_over = new EXP_OVER(sender_id, num_bytes, buff);
		WSAOVERLAPPED wsa_over;
		ZeroMemory(&wsa_over, sizeof(wsa_over));
		
		WSASend(socket, &buff, 1, 0, 0, &wsa_over, send_callback);
	}
};

unordered_map <unsigned long long, SESSION> clients;

void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED send_over, DWORD recv_flag)
{
	//delete reinterpret_cast<EXP_OVER*>(send_over);
}

void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED recv_over, DWORD recv_flag)	
{
	game.update();
	//recv하고나서 클라한테 받은 데이터 적용
	unsigned long long client_id = reinterpret_cast<unsigned long long>(recv_over->hEvent);
	//cout << "Client" << client_id << " Sent[" << num_bytes << "bytes] : " << clients[client_id].recv_buffer << endl;

	//모든 클라한테 데이터 전송
	for (auto& client : clients) {
		//client.second.do_send(client_id, num_bytes, clients[client_id].recv_wsa_buff);
		WSABUF send_wsa_buff;
		send_wsa_buff.buf = (char*)&game.player->position;
		send_wsa_buff.len = sizeof(game.player->position);
		client.second.do_send(client_id, num_bytes, send_wsa_buff);
	}
	
	//수신
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
	}
	
	clients.clear();
	closesocket(server_socket);
	WSACleanup();
}