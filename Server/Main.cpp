#include <iostream>
#include <unordered_map>
#include <random>
#include "Player.h"
#pragma comment (lib, "WS2_32.LIB")
using namespace std;

const short SERVER_PORT = 9000;
const int BUFSIZE = 4000;

void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED send_over, DWORD recv_flag);
void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED recv_over, DWORD recv_flag);
void delete_session(int);
TI randomly_spawn_player();
void error_handling(int);

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
	Player player{ randomly_spawn_player(), client_id };

	SESSION() {
		cout << "Unexpected Constructor Call Error!\n";
		exit(-1);
	}
	
	SESSION(int id, SOCKET s) : client_id(id), socket(s) {
		recv_wsa_buff.buf = (char*)&player.key_input;
		recv_wsa_buff.len = sizeof(player.key_input);
	}
	
	~SESSION() { 
		closesocket(socket);
	}

	bool do_recv() {
		DWORD recv_flag = 0;
		ZeroMemory(&recv_over, sizeof(recv_over));
		recv_over.hEvent = reinterpret_cast<HANDLE>(client_id);
		
		int retval = WSARecv(socket, &recv_wsa_buff, 1, 0, &recv_flag, &recv_over, recv_callback);
		if (retval == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
			cout << "WSARecv() failed with error " << WSAGetLastError() << endl;
			error_handling(client_id);
			return 1;
		}
		return 0;
	}
	
	bool do_send(int root_client_id, TI data) {
		PACKET* exp_over = new PACKET{ client_id, root_client_id, data };
		
		int retval = WSASend(socket, &exp_over->send_wsa_buf, 1, 0, 0, &exp_over->send_over, send_callback);
		if (retval == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
			cout << "WSASend() failed with error " << WSAGetLastError() << endl;
			error_handling(client_id);
			delete exp_over;
			return 1;
		}
		return 0;
	}
};

unordered_map <int, SESSION> clients;

void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED send_over, DWORD recv_flag)
{
	PACKET* exp_over = reinterpret_cast<PACKET*>(send_over);	//PACKET 클래스의 첫번째 변수 주소를 가지고 PACKET 클래스 포인터를 찾음
	int sent_client_id = reinterpret_cast<int>(exp_over->send_over.hEvent);
	delete exp_over;

	if (err != 0) {
		cout << "Send Callback Error " << err << " Client_ID: " << sent_client_id << endl;
		error_handling(sent_client_id);
		return;
	}
}

void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED recv_over, DWORD recv_flag)	
{
	int client_id = reinterpret_cast<int>(recv_over->hEvent);

	if (err != 0) {
		cout << "Recv Callback Error: " << err << " Client_ID: " << client_id << endl;
		error_handling(client_id);
		return;
	}

	clients[client_id].player.key_check();
	for (auto& client : clients) {	//모든 클라한테 데이터 전송
		//cout << "SEND TO " << client.first << " FROM " << root_client_id << endl;
		if (client.second.do_send(client_id, clients[client_id].player.position)) return;
	}
	
	if (clients[client_id].do_recv()) return;
}

void delete_session(int client_id) 
{
	cout << "DELETE SESSION: " << client_id << endl;
	clients.erase(client_id);
}

void error_handling(int client_id)
{
	cout << "ERROR HANDLING: " << client_id << endl;
	for (auto& client : clients) {
		if (client.first == client_id) continue;
		client.second.do_send(client_id, TI{ -1, -1 });
	}
	delete_session(client_id);
}

TI randomly_spawn_player()
{
	random_device rd;
	default_random_engine dre(rd());
	uniform_int_distribution <int>spawn_location(0, 7);
	do {
		bool same_location = false;
		TI player_location{ spawn_location(dre) * 100 + 50, spawn_location(dre) * 100 + 50 };
		if (clients.size() >= 64) {		//칸이 64개니까 64명 넘어가면 걍 겹치게 둠
			return player_location;
		}
		for (auto& client : clients) {	//위치 겹치면 랜덤 다시 돌림
			if (client.second.player.position.x == player_location.x && client.second.player.position.y == player_location.y) {
				same_location = true;
				break;
			}
		}
		if (!same_location) return player_location;
	} while (true);
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
	
	for (;;) {
		SOCKET client_socket = WSAAccept(server_socket, reinterpret_cast<sockaddr*>(&server_addr), &addr_size, 0, 0);
		for (int i = 1; ; i++) {
			if (clients.find(i) == clients.end()) {
				clients.try_emplace(i, i, client_socket);
				clients[i].do_recv();
				cout << "Client added: " << i << endl;
				break;
			}
		}
	}
	
	clients.clear();
	closesocket(server_socket);
	WSACleanup();
}