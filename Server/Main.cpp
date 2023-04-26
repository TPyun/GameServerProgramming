#include <array>
#include <random>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_set>
#include <concurrent_unordered_set.h>
#include "Player.h"


#include <Windows.h>
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>

#ifdef _DEBUG
#define new new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#endif

using namespace std;




////Overlapped IO
//void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED send_over, DWORD recv_flag);
//void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED recv_over, DWORD recv_flag);
//void delete_session(int);
//TI randomly_spawn_player();
//void error_handling(int);
//
//class OVER_EXP {
//public:
//	WSAOVERLAPPED send_over;	//클래스의 맨 첫번째에 있어야만 함. 이거를 가지고 클래스 포인터 위치 찾을거임
//	int root_client_id;
//	WSABUF send_wsa_buf;
//	char send_data[BUFSIZE];
//	
//	OVER_EXP(int client_id, char* packet)
//	{
//		ZeroMemory(&send_over, sizeof(send_over));
//		send_over.hEvent = reinterpret_cast<HANDLE>(client_id);
//		send_wsa_buf.len = packet[0];
//		send_wsa_buf.buf = packet;
//	}
//
//	~OVER_EXP() {}
//};
//
//class SESSION {
//private:
//	SOCKET socket;
//	WSAOVERLAPPED recv_over;
//	int client_id;
//public:
//	char recv_data[BUFSIZE];
//	WSABUF recv_wsa_buff;
//	Player player{ randomly_spawn_player(), client_id };
//
//	SESSION() {
//		cout << "Unexpected Constructor Call Error!\n";
//		exit(-1);
//	}
//	
//	SESSION(int id, SOCKET s) : client_id(id), socket(s) {
//		recv_wsa_buff.len = BUFSIZE;
//		recv_wsa_buff.buf = recv_data;
//	}
//	
//	~SESSION() { 
//		closesocket(socket);
//	}
//
//	bool do_recv() {
//		cout << "do_recv() called\n";
//		DWORD recv_flag = 0;
//		ZeroMemory(&recv_over, sizeof(recv_over));
//		recv_over.hEvent = reinterpret_cast<HANDLE>(client_id);
//		
//		int retval = WSARecv(socket, &recv_wsa_buff, 1, 0, &recv_flag, &recv_over, recv_callback);
//		if (retval == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
//			cout << "WSARecv() failed with error " << WSAGetLastError() << endl;
//			error_handling(client_id);
//			return 1;
//		}
//		cout << "do_recv() finished\n";
//		return 0;
//	}
//	
//	bool do_send(char* packet) {
//		cout << "do_send() called\n";
//		OVER_EXP* exp_over = new OVER_EXP{ client_id, packet };
//		
//		int retval = WSASend(socket, &exp_over->send_wsa_buf, 1, 0, 0, &exp_over->send_over, send_callback);
//		if (retval == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
//			cout << "WSASend() failed with error " << WSAGetLastError() << endl;
//			error_handling(client_id);
//			delete exp_over;
//			return 1;
//		}
//		return 0;
//	}
//
//	void send_login_packet();
//	void send_move_packet(int);
//	void send_out_packet(int);
//};
//
//unordered_map <int, SESSION> clients;
//
//void SESSION::send_login_packet()
//{
//	SC_LOGIN_PACKET packet;
//	packet.client_id = client_id;
//	packet.position = player.position;
//	do_send((char*)&packet);
//}
//
//void SESSION::send_move_packet(int client_id)
//{
//	SC_MOVE_PACKET packet;
//	packet.client_id = client_id;
//	packet.position = clients[client_id].player.position;
//	do_send((char*)&packet);
//}
//
//void SESSION::send_out_packet(int client_id)
//{
//	SC_OUT_PACKET packet;
//	packet.client_id = client_id;
//	do_send((char*) & packet);
//}
//
//void process_packet(int client_id, char* packet)
//{
//	switch (packet[1]) {
//	case CS_MOVE:
//	{
//		SESSION* this_client = &clients[client_id];
//		CS_MOVE_PACKET* recv_packet = reinterpret_cast<CS_MOVE_PACKET*>(packet);
//		memcpy(&this_client->player.key_input, &recv_packet->ks, sizeof(recv_packet->ks));
//		this_client->player.key_check();
//		for (auto& client : clients) {	//모든 클라한테 데이터 전송
//			client.second.send_move_packet(client_id);
//		}
//		break;
//	}
//	case CS_LOGIN:
//	{
//		SESSION* this_client = &clients[client_id];
//		CS_LOGIN_PACKET* recv_packet = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
//
//		this_client->send_login_packet();
//		for (auto& client : clients) {	//새로 들어온 클라에게 기존 애들 위치 전송
//			if (client_id == client.first)
//				continue;
//			this_client->send_move_packet(client.first);
//		}
//		for (auto& client : clients) {	//모든 클라한테 새로온 애 위치 전송
//			if (client_id == client.first)
//				continue;
//			client.second.send_move_packet(client_id);
//		}
//		break;
//	}
//	}
//}
//
//void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED send_over, DWORD recv_flag)
//{
//	OVER_EXP* exp_over = reinterpret_cast<OVER_EXP*>(send_over);	//PACKET 클래스의 첫번째 변수 주소를 가지고 PACKET 클래스 포인터를 찾음
//	int sent_client_id = reinterpret_cast<int>(exp_over->send_over.hEvent);
//	delete exp_over;
//
//	if (err != 0) {
//		cout << "Send Callback Error " << err << " Client_ID: " << sent_client_id << endl;
//		error_handling(sent_client_id);
//		return;
//	}
//}
//
//void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED recv_over, DWORD recv_flag)	
//{
//	int client_id = reinterpret_cast<int>(recv_over->hEvent);
//
//	if (err != 0) {
//		cout << "Recv Callback Error: " << err << " Client_ID: " << client_id << endl;
//		error_handling(client_id);
//		return;
//	}
//	
//	process_packet(client_id, (char*)&clients[client_id].recv_data);
//	if (clients[client_id].do_recv()) return;
//}
//
//void delete_session(int client_id) 
//{
//	cout << "DELETE SESSION: " << client_id << endl;
//	clients.erase(client_id);
//}
//
//void error_handling(int client_id)
//{
//	cout << "ERROR HANDLING: " << client_id << endl;
//	for (auto& client : clients) {
//		if (client.first == client_id) continue;
//		SC_OUT_PACKET out_packet;
//		out_packet.client_id = client_id;
//		client.second.do_send((char*)&out_packet);
//	}
//	delete_session(client_id);
//}
//
//TI randomly_spawn_player()
//{
//	random_device rd;
//	default_random_engine dre(rd());
//	uniform_int_distribution <int>spawn_location(0, 7);
//	do {
//		bool same_location = false;
//		TI player_location{ spawn_location(dre) * 100 + 50, spawn_location(dre) * 100 + 50 };
//		if (clients.size() >= 64) {		//칸이 64개니까 64명 넘어가면 걍 겹치게 둠
//			return player_location;
//		}
//		for (auto& client : clients) {	//위치 겹치면 랜덤 다시 돌림
//			if (client.second.player.position.x == player_location.x && client.second.player.position.y == player_location.y) {
//				same_location = true;
//				break;
//			}
//		}
//		if (!same_location) return player_location;
//	} while (true);
//}
//
//int main()
//{
//	WSADATA WSAData;
//	WSAStartup(MAKEWORD(2, 2), &WSAData);
//	SOCKET server_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
//	SOCKADDR_IN server_addr;
//	ZeroMemory(&server_addr, sizeof(server_addr));
//	server_addr.sin_family = AF_INET;
//	server_addr.sin_port = htons(SERVER_PORT);
//	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
//	bind(server_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
//	listen(server_socket, SOMAXCONN);
//	INT addr_size = sizeof(server_addr);
//	
//	cout << "Port: " << SERVER_PORT << endl;
//	
//	for (;;) {
//		SOCKET client_socket = WSAAccept(server_socket, reinterpret_cast<sockaddr*>(&server_addr), &addr_size, 0, 0);
//		for (int i = 1; ; i++) {
//			if (clients.find(i) == clients.end()) {
//				clients.try_emplace(i, i, client_socket);
//				clients[i].do_recv();
//				cout << "Client added: " << i << endl;
//				break;
//			}
//		}
//	}
//	
//	clients.clear();
//	closesocket(server_socket);
//	WSACleanup();
//}







//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////IOCP SINGLE THREAD
//void delete_session(int);
//TI randomly_spawn_player();
//void disconnect(int);
//enum COMPLETION_TYPE { OP_ACCEPT, OP_RECV, OP_SEND };
//
//class OVER_EXP {
//public:
//	WSAOVERLAPPED over;	//클래스의 맨 첫번째에 있어야만 함. 이거를 가지고 클래스 포인터 위치 찾을거임
//	int root_client_id;
//	WSABUF wsa_buf;
//	char data[BUFSIZE];
//	COMPLETION_TYPE completion_type;
//
//	OVER_EXP()				//Recv
//	{
//		ZeroMemory(&over, sizeof(over));
//		ZeroMemory(data, sizeof(data));
//		wsa_buf.buf = data;
//		wsa_buf.len = BUFSIZE;
//		completion_type = OP_RECV;
//	}
//
//	OVER_EXP(char* packet)	//Send
//	{
//		ZeroMemory(&over, sizeof(over));
//		wsa_buf.buf = packet;
//		wsa_buf.len = packet[0];
//		completion_type = OP_SEND;
//	}
//
//	~OVER_EXP() {}
//};
//
//class SESSION {
//	OVER_EXP recv_over;
//
//private:
//	SOCKET socket;
//	int client_id{};
//
//public:
//	int	remain_data{};
//	Player player;
//
//	SESSION() {
//		cout << "Unexpected Constructor Call Error!\n";
//		exit(-1);
//	}
//
//	SESSION(int id, SOCKET s) : client_id(id), socket(s) {
//
//	}
//
//	~SESSION() {
//		closesocket(socket);
//	}
//
//	bool do_recv() {
//		DWORD recv_flag = 0;
//		memset(&recv_over.over, 0, sizeof(recv_over.over));
//		recv_over.wsa_buf.len = BUFSIZE - remain_data;
//		recv_over.wsa_buf.buf = recv_over.data + remain_data;;
//		int retval = WSARecv(socket, &recv_over.wsa_buf, 1, 0, &recv_flag,&recv_over.over, 0);
//		if (retval == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
//			cout << "WSARecv() failed with error " << WSAGetLastError() << endl;
//			disconnect(client_id);
//			return 1;
//		}
//		return 0;
//	}
//
//	bool do_send(void* packet) {
//		OVER_EXP* send_over = new OVER_EXP{ reinterpret_cast<char*>(packet) };
//
//		int retval = WSASend(socket, &send_over->wsa_buf, 1, 0, 0, &send_over->over, 0);
//		if (retval == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
//			cout << "WSASend() failed with error " << WSAGetLastError() << endl;
//			disconnect(client_id);
//			delete send_over;
//			return 1;
//		}
//		return 0;
//	}
//	
//	void send_login_packet();
//	void send_move_packet(int);
//	void send_out_packet(int);
//};
//
//unordered_map <int, SESSION> clients;
//
//void SESSION::send_login_packet()
//{
//	SC_LOGIN_PACKET packet;
//	packet.client_id = client_id;
//	packet.position = player.position;
//	do_send(&packet);
//}
//
//void SESSION::send_move_packet(int client_id)
//{
//	SC_MOVE_PACKET packet;
//	packet.client_id = client_id;
//	packet.position = clients[client_id].player.position;
//	do_send(&packet);
//}
//
//void SESSION::send_out_packet(int client_id)
//{
//	SC_OUT_PACKET packet;
//	packet.client_id = client_id;
//	do_send(&packet);
//}
//
//void delete_session(int client_id)
//{
//	cout << "DELETE SESSION: " << client_id << endl;
//	clients.erase(client_id);
//}
//
//void disconnect(int client_id)
//{
//	cout << "ERROR HANDLING: " << client_id << endl;
//	for (auto& client : clients) {
//		if (client.first == client_id) continue;
//		SC_OUT_PACKET out_packet;
//		out_packet.client_id = client_id;
//		client.second.do_send(&out_packet);
//	}
//	delete_session(client_id);
//}
//
//TI randomly_spawn_player()
//{
//	random_device rd;
//	default_random_engine dre(rd());
//	uniform_int_distribution <int>spawn_location(0, 7);
//	do {
//		bool same_location = false;
//		TI player_location{ spawn_location(dre) * 100 + 50, spawn_location(dre) * 100 + 50 };
//		if (clients.size() >= 64) {		//칸이 64개니까 64명 넘어가면 걍 겹치게 둠
//			return player_location;
//		}
//		for (auto& client : clients) {	//위치 겹치면 랜덤 다시 돌림
//			if (client.second.player.position.x == player_location.x && client.second.player.position.y == player_location.y) {
//				same_location = true;
//				break;
//			}
//		}
//		if (!same_location) return player_location;
//	} while (true);
//}
// 
//int get_new_client_id()
//{
//	for (int i = 1; ; i++) {
//		if (clients.find(i) == clients.end()) return i;
//	}
//	return -1;
//}
//
//void process_packet(int client_id, char* packet)
//{
//	switch (packet[1]) {
//	case CS_MOVE: 
//	{
//		SESSION* this_client = &clients[client_id];
//		CS_MOVE_PACKET* recv_packet = reinterpret_cast<CS_MOVE_PACKET*>(packet);
//		memcpy(&this_client->player.key_input, &recv_packet->ks, sizeof(recv_packet->ks));
//		this_client->player.key_check();
//		for (auto& client : clients) {	//모든 클라한테 데이터 전송
//			client.second.send_move_packet(client_id);
//		}
//		break;
//	}
//	case CS_LOGIN:
//	{
//		SESSION* this_client = &clients[client_id];
//		CS_LOGIN_PACKET* recv_packet = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
//		
//		this_client->send_login_packet();
//		for (auto& client : clients) {	//새로 들어온 클라에게 기존 애들 위치 전송
//			if (client_id == client.first)
//				continue;
//			this_client->send_move_packet(client.first);
//		}
//		for (auto& client : clients) {	//모든 클라한테 새로온 애 위치 전송
//			if (client_id == client.first)
//				continue;
//			client.second.send_move_packet(client_id);
//		}
//		break;
//	}
//	}
//}
//
//int main()
//{
//	WSADATA WSAData;
//	WSAStartup(MAKEWORD(2, 2), &WSAData);
//	SOCKET server_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
//	SOCKADDR_IN server_addr;
//	ZeroMemory(&server_addr, sizeof(server_addr));
//	server_addr.sin_family = AF_INET;
//	server_addr.sin_port = htons(SERVER_PORT);
//	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
//	bind(server_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
//	listen(server_socket, SOMAXCONN);
//	INT addr_size = sizeof(server_addr);
//
//	cout << "Port: " << SERVER_PORT << endl;
//
//	HANDLE h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
//	CreateIoCompletionPort(reinterpret_cast<HANDLE>(server_socket), h_iocp, 9999, 0);
//	SOCKET client_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
//	OVER_EXP accept_over;
//	accept_over.completion_type = OP_ACCEPT;
//	AcceptEx(server_socket, client_socket, accept_over.data, 0, addr_size + 16, addr_size + 16, 0, &accept_over.over);
//
//
//	for (;;) {
//		DWORD num_bytes;
//		ULONG_PTR key;
//		WSAOVERLAPPED* over = nullptr;
//		BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);
//		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);
//		if (FALSE == ret) {
//			if (ex_over->completion_type == OP_ACCEPT) cout << "Accept Error";
//			else {
//				cout << "GQCS Error on client[" << key << "]\n";
//				disconnect(static_cast<int>(key));
//				if (ex_over->completion_type == OP_SEND) delete ex_over;
//				continue;
//			}
//		}
//
//		switch (ex_over->completion_type) {
//		case OP_ACCEPT: {
//			int client_id = get_new_client_id();
//			if (client_id != -1) {
//				clients.try_emplace(client_id, client_id, client_socket);
//				
//				CreateIoCompletionPort(reinterpret_cast<HANDLE>(client_socket),h_iocp, client_id, 0);
//				clients[client_id].do_recv();
//				client_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
//			}
//			else {
//				cout << "Max user exceeded.\n";
//			}
//			ZeroMemory(&accept_over.over, sizeof(accept_over.over));
//			AcceptEx(server_socket, client_socket, accept_over.data, 0, addr_size + 16, addr_size + 16, 0, &accept_over.over);
//			break;
//		}
//		case OP_RECV: {
//			int data_to_proccess = num_bytes + clients[key].remain_data;
//			char* p = ex_over->data;
//			while (data_to_proccess > 0) {
//				int packet_size = p[0];
//				if (packet_size <= data_to_proccess) {
//					process_packet(static_cast<int>(key), p);
//					p += packet_size;
//					data_to_proccess -= packet_size;
//				}
//				else break;
//			}
//			clients[key].remain_data = data_to_proccess;
//			if (data_to_proccess > 0) {
//				memcpy(ex_over->data, p, data_to_proccess);
//			}
//			clients[key].do_recv();
//			break;
//		}
//		case OP_SEND:
//			delete ex_over;
//			break;
//		}
//	}
//
//	clients.clear();
//	closesocket(server_socket);
//	WSACleanup();
//}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////ICOP Sigle Thread 
//TI randomly_spawn_player();
//void disconnect(int);
//int over_num = 0;
//
//enum COMPLETION_TYPE { OP_ACCEPT, OP_RECV, OP_SEND };
//class OVER_EXP {
//public:
//	WSAOVERLAPPED over;	//클래스의 맨 첫번째에 있어야만 함. 이거를 가지고 클래스 포인터 위치 찾을거임
//	WSABUF wsa_buf;
//	char data[BUFSIZE];
//	COMPLETION_TYPE completion_type;
//
//	OVER_EXP()				//Recv
//	{
//		ZeroMemory(&over, sizeof(over));
//		ZeroMemory(&data, sizeof(data));
//		wsa_buf.len = BUFSIZE;
//		wsa_buf.buf = data;
//		completion_type = OP_RECV;
//	}
//
//	OVER_EXP(char* packet)	//Send
//	{
//		ZeroMemory(&over, sizeof(over));
//		wsa_buf.len = packet[0];
//		wsa_buf.buf = packet;
//		completion_type = OP_SEND;
//	}
//	~OVER_EXP()
//	{
//	}
//};
//
//enum SESSION_STATE { FREE, ALLOC, INGAME };
//class SESSION {
//	OVER_EXP recv_over;
//
//public:
//	SESSION_STATE state;
//	mutex state_mtx;
//	SOCKET socket;
//	int client_id{};
//	unsigned int prev_time{};
//	int	remain_data{};
//	Player player;
//
//	SESSION() {
//		state = FREE;
//		socket = INVALID_SOCKET;
//		client_id = -1;
//		prev_time = 0;
//		remain_data = 0;
//	}
//
//	~SESSION() {}
//
//	void do_recv() {
//		//cout << client_id << " do_recv\n";
//		DWORD recv_flag = 0;
//		memset(&recv_over.over, 0, sizeof(recv_over.over));
//		recv_over.wsa_buf.len = BUFSIZE - remain_data;
//		recv_over.wsa_buf.buf = recv_over.data + remain_data;
//		WSARecv(socket, &recv_over.wsa_buf, 1, 0, &recv_flag, &recv_over.over, 0);
//	}
//
//	void do_send(void* packet) {
//		//cout << client_id << " do_send\n";
//		OVER_EXP* send_over = new OVER_EXP{ reinterpret_cast<char*>(packet) };
//		WSASend(socket, &send_over->wsa_buf, 1, 0, 0, &send_over->over, 0);
//		//printf(" alloc %p\n", send_over);
//	}
//
//	void send_login_packet();
//	void send_move_packet(int);
//	void send_out_packet(int);
//};
//
//array<SESSION, MAX_USER> clients;
//
//void SESSION::send_login_packet()
//{
//	SC_LOGIN_PACKET packet;
//	packet.client_id = client_id;
//	//player.position = randomly_spawn_player();
//	player.position.x = rand() % MAP_SIZE;
//	player.position.y = rand() % MAP_SIZE;
//
//	packet.position = player.position;
//	do_send(&packet);
//}
//
//void SESSION::send_move_packet(int client_id)
//{
//	SC_MOVE_PACKET packet;
//	packet.client_id = client_id;
//	packet.position = clients[client_id].player.position;
//	packet.time = prev_time;
//	do_send(&packet);
//}
//
//void SESSION::send_out_packet(int client_id)
//{
//	SC_OUT_PACKET packet;
//	packet.client_id = client_id;
//	do_send(&packet);
//}
//
//void disconnect(int client_id)
//{
//	//cout << "ERROR HANDLING: " << client_id << endl;
//	int client_num = 0;
//	for (auto& client : clients) {
//		{
//			lock_guard<mutex> m(client.state_mtx);
//			if (INGAME != client.state) continue;
//		}
//		client_num++;
//		if (client.client_id == client_id) continue;
//		client.send_out_packet(client_id);
//		//cout << "remain client: " << client.client_id << endl;
//	}
//	cout << client_num << endl;
//	closesocket(clients[client_id].socket);
//
//	lock_guard<mutex> m{ clients[client_id].state_mtx };
//	clients[client_id].state = FREE;
//}
//
//TI randomly_spawn_player()
//{
//	random_device rd;
//	default_random_engine dre(rd());
//	uniform_int_distribution <int>spawn_location(0, MAP_SIZE);
//	do {
//		bool same_location = false;
//		TI player_location{ spawn_location(dre) * BLOCK_SIZE + BLOCK_SIZE / 2, spawn_location(dre) * BLOCK_SIZE + BLOCK_SIZE / 2 };
//		if (clients.size() >= MAP_SIZE * MAP_SIZE) {		//칸이 64개니까 64명 넘어가면 걍 겹치게 둠
//			return player_location;
//		}
//		for (auto& client : clients) {	//위치 겹치면 랜덤 다시 돌림
//			if (client.player.position.x == player_location.x && client.player.position.y == player_location.y) {
//				same_location = true;
//				break;
//			}
//		}
//		if (!same_location) return player_location;
//	} while (true);
//}
//
//bool in_eyesight(int p1, int p2)
//{
//	if (abs(clients[p1].player.position.x - clients[p2].player.position.x) > VIEW_RANGE) return false;
//	if (abs(clients[p1].player.position.y - clients[p2].player.position.y) > VIEW_RANGE) return false;
//	return true;
//}
//
//int get_new_client_id()
//{
//	for (int i = 0; i < clients.size(); i++) {
//		lock_guard<mutex> m(clients[i].state_mtx);
//		if (clients[i].state == FREE) return i;
//	}
//	return -1;
//}
//
//void process_packet(int client_id, char* packet)
//{
//	switch (packet[1]) {
//	case CS_MOVE:
//	{
//		SESSION* moved_client = &clients[client_id];
//		CS_MOVE_PACKET* recv_packet = reinterpret_cast<CS_MOVE_PACKET*>(packet);
//		memcpy(&moved_client->player.key_input, &recv_packet->ks, sizeof(recv_packet->ks));
//		moved_client->player.key_check();
//		moved_client->prev_time = recv_packet->time;
//
//		for (auto& client : clients) {
//			{
//				lock_guard<mutex> m(clients[client_id].state_mtx);
//				if (client.state != INGAME) continue;
//			}
//			if (in_eyesight(client_id, client.client_id)) {
//				client.send_move_packet(client_id);
//			}
//			else {
//				client.send_out_packet(client_id);
//				moved_client->send_out_packet(client.client_id);
//			}
//		}
//		break;
//	}
//	case CS_LOGIN:
//	{
//		SESSION* new_client = &clients[client_id];
//		CS_LOGIN_PACKET* recv_packet = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
//		new_client->send_login_packet();	//새로온 애한테 로그인 됐다고 전송
//		{
//			lock_guard<mutex> m{ clients[client_id].state_mtx };
//			clients[client_id].state = INGAME;
//		}
//		for (auto& old_client : clients) {
//			{
//				lock_guard<mutex> m(clients[client_id].state_mtx);
//				if (old_client.state != INGAME) continue;
//			}
//			if (client_id == old_client.client_id) continue;
//			if (in_eyesight(client_id, old_client.client_id)) {
//				new_client->send_move_packet(old_client.client_id);	//새로 들어온 클라에게 시야 안의 기존 애들 위치 전송
//				old_client.send_move_packet(client_id); //시야 안에 클라한테 새로온 애 위치 전송
//			}
//		}
//		break;
//	}
//	default: cout << "Unknown Packet Type" << endl; break;
//	}
//}
//
//int main()
//{
//	WSADATA WSAData;
//	HANDLE h_iocp;
//	WSAStartup(MAKEWORD(2, 2), &WSAData);
//	SOCKET server_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
//	SOCKADDR_IN server_addr;
//	ZeroMemory(&server_addr, sizeof(server_addr));
//	server_addr.sin_family = AF_INET;
//	server_addr.sin_port = htons(SERVER_PORT);
//	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
//	bind(server_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
//	listen(server_socket, SOMAXCONN);
//	SOCKADDR_IN client_addr;
//	int addr_size = sizeof(client_addr);
//	cout << "Port: " << SERVER_PORT << endl;
//
//	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
//	CreateIoCompletionPort(reinterpret_cast<HANDLE>(server_socket), h_iocp, 9999, 0);
//	SOCKET client_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
//	OVER_EXP accept_over;
//	accept_over.completion_type = OP_ACCEPT;
//	AcceptEx(server_socket, client_socket, accept_over.data, 0, addr_size + 16, addr_size + 16, 0, &accept_over.over);
//
//	while (true) {
//		DWORD num_bytes;
//		ULONG_PTR key;
//		WSAOVERLAPPED* over = nullptr;
//		BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);
//		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);
//		if (FALSE == ret) {
//			if (ex_over->completion_type == OP_ACCEPT) cout << "Accept Error";
//			else {
//				//cout << "GQCS Error on client[" << key << "]\n";
//				disconnect(static_cast<int>(key));
//				if (ex_over->completion_type == OP_SEND) delete ex_over;
//				continue;
//			}
//		}
//
//		if ((0 == num_bytes) && ((ex_over->completion_type == OP_RECV) || (ex_over->completion_type == OP_SEND))) {
//			cout << "GQCS Error on client[" << key << "]\n";
//			disconnect(static_cast<int>(key));
//			if (ex_over->completion_type == OP_SEND) delete ex_over;
//			continue;
//		}
//
//		switch (ex_over->completion_type)
//		{
//		case OP_SEND:
//		{
//			//printf("delete %p\n\n", ex_over);
//			delete ex_over;
//			//_CrtDumpMemoryLeaks();
//			break;
//		}
//		case OP_RECV:
//		{
//			int data_to_proccess = num_bytes + clients[key].remain_data;
//			char* p = ex_over->data;
//			while (data_to_proccess > 0) {
//				int packet_size = p[0];
//				if (packet_size <= data_to_proccess) {
//					process_packet(static_cast<int>(key), p);
//					p += packet_size;
//					data_to_proccess -= packet_size;
//				}
//				else break;
//			}
//			clients[key].remain_data = data_to_proccess;
//			if (data_to_proccess > 0) {
//				memcpy(ex_over->data, p, data_to_proccess);
//			}
//			clients[key].do_recv();
//			break;
//		}
//		case OP_ACCEPT:
//		{
//			int client_id = get_new_client_id();
//			if (client_id != -1) {
//				{
//					lock_guard<mutex> m(clients[client_id].state_mtx);
//					clients[client_id].state = ALLOC;
//				}
//				clients[client_id].client_id = client_id;
//				clients[client_id].socket = client_socket;
//				clients[client_id].remain_data = 0;
//
//				CreateIoCompletionPort(reinterpret_cast<HANDLE>(client_socket), h_iocp, client_id, 0);
//				clients[client_id].do_recv();
//				client_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
//			}
//			else {
//				cout << "Max user exceeded.\n";
//			}
//			ZeroMemory(&accept_over.over, sizeof(accept_over.over));
//			int addr_size = sizeof(SOCKADDR_IN);
//			AcceptEx(server_socket, client_socket, accept_over.data, 0, addr_size + 16, addr_size + 16, 0, &accept_over.over);
//			break;
//		}
//		default: cout << "Unknown Completion Type" << endl; break;
//		}
//	}
//
//	closesocket(server_socket);
//	WSACleanup();
//}
//








////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//ICOP Multi Thiread 

enum COMPLETION_TYPE { OP_ACCEPT, OP_RECV, OP_SEND };
class OVER_EXP {
public:
	WSAOVERLAPPED over;	//클래스의 맨 첫번째에 있어야만 함. 이거를 가지고 클래스 포인터 위치 찾을거임
	WSABUF wsa_buf;
	char data[BUFSIZE];
	COMPLETION_TYPE completion_type;

	OVER_EXP()				//Recv
	{
		wsa_buf.len = BUFSIZE;
		wsa_buf.buf = data;
		completion_type = OP_RECV;
		ZeroMemory(&over, sizeof(over));
	}
	OVER_EXP(char* packet)	//Send
	{
		wsa_buf.len = packet[0];
		wsa_buf.buf = data;
		ZeroMemory(&over, sizeof(over));
		completion_type = OP_SEND;
		memcpy(data, packet, packet[0]);
	}
	~OVER_EXP() {}
};

OVER_EXP global_accept_over;
SOCKET global_client_socket;
SOCKET global_server_socket;
TI randomly_spawn_player();
void disconnect(int);
atomic<int> connected_players = 0;

enum SESSION_STATE { FREE, ALLOC, INGAME };
class SESSION {
	OVER_EXP recv_over;
	
public:
	SOCKET socket;
	int client_id;
	unsigned int prev_time;
	int	remain_data;
	Player player;
	
	SESSION_STATE state;
	mutex state_mtx;
	unordered_set<int> view_list;
	mutex view_list_mtx;

	SESSION() {
		state = FREE;
		socket = 0;
		client_id = -1;
		prev_time = 0;
		remain_data = 0;
	}
	~SESSION() {}

	void do_recv() {
		//cout << client_id << " do_recv\n";
		DWORD recv_flag = 0;
		memset(&recv_over.over, 0, sizeof(recv_over.over));
		recv_over.wsa_buf.len = BUFSIZE - remain_data;
		recv_over.wsa_buf.buf = recv_over.data + remain_data;
		int ret = WSARecv(socket, &recv_over.wsa_buf, 1, 0, &recv_flag,&recv_over.over, 0);
		if (0 != ret) {
			int err_no = WSAGetLastError();
			if (WSA_IO_PENDING != err_no) {
				//cout << "WSARecv Error : " << err_no << " Client: " << client_id << endl;
				bool is_ingame = false;
				{
					lock_guard<mutex> m{ state_mtx };
					if (state == INGAME) is_ingame = true;
				}
				if (is_ingame)
					disconnect(client_id);
			}
		}
	}
		
	void do_send(void* packet) {
		OVER_EXP* send_over = new OVER_EXP(reinterpret_cast<char*>(packet));
		int ret = WSASend(socket, &send_over->wsa_buf, 1, 0, 0, &send_over->over, 0);
		if (0 != ret) {
			int err_no = WSAGetLastError();
			if (WSA_IO_PENDING != err_no) {
				//cout << "WSASend Error : " << err_no << " Client: " << client_id << endl;
				delete send_over;
				/*bool is_ingame = false;
				{
					lock_guard<mutex> m{ state_mtx };
					if (state == INGAME) is_ingame = true;
				}
				if(is_ingame)
					disconnect(client_id);*/
			}
		}
	}
	
	void send_login_packet();
	void send_move_packet(int);
	void send_out_packet(int);
	void insert_view_list(int);
	void erase_view_list(int);
};

array<SESSION, MAX_USER> clients;

void SESSION::send_login_packet()
{
	connected_players.fetch_add(1);
	cout << connected_players << endl;

	player.position = randomly_spawn_player();
	SC_LOGIN_PACKET packet;
	packet.client_id = client_id;
	packet.position = player.position;
	do_send(&packet);
}

void SESSION::send_move_packet(int client_id)
{
	SC_MOVE_PACKET packet;
	packet.client_id = client_id;
	packet.position = clients[client_id].player.position;
	packet.time = clients[client_id].prev_time;
	do_send(&packet);
}

void SESSION::send_out_packet(int client_id)
{
	SC_OUT_PACKET packet;
	packet.client_id = client_id;
	do_send(&packet);
}

void SESSION::insert_view_list(int new_client_id)
{
	view_list_mtx.lock();
	view_list.insert(new_client_id);
	view_list_mtx.unlock();
}

void SESSION::erase_view_list(int new_client_id)
{
	view_list_mtx.lock();
	view_list.erase(new_client_id);
	view_list_mtx.unlock();
}

void disconnect(int client_id)
{
	{
		lock_guard<mutex> m{ clients[client_id].state_mtx };
		clients[client_id].state = FREE;
	}
	clients[client_id].view_list_mtx.lock();
	unordered_set<int> view_list = clients[client_id].view_list;
	clients[client_id].view_list_mtx.unlock();

	for (auto& client_in_view : view_list) {
		clients[client_in_view].send_out_packet(client_id);
	}
	closesocket(clients[client_id].socket);

	int remain = 0;
	for (auto& client : clients) {
		lock_guard<mutex> m{ clients[client_id].state_mtx };
		if (client.state == INGAME) remain++;
	}
	cout << "REMAIN: " << remain << endl;
	
	connected_players.fetch_sub(1);
	cout << connected_players << endl;
}

TI randomly_spawn_player()
{
	random_device rd;
	default_random_engine dre(rd());
	uniform_int_distribution <int>spawn_location(0, MAP_SIZE);
	do {
		bool same_location = false;
		TI player_location{ spawn_location(dre), spawn_location(dre)};
		if (!same_location) return player_location;
	} while (true);
}

bool in_eyesight(int p1, int p2)
{
	if (abs(clients[p1].player.position.x - clients[p2].player.position.x) > VIEW_RANGE) return false;
	if (abs(clients[p1].player.position.y - clients[p2].player.position.y) > VIEW_RANGE) return false;
	return true;
}

int get_new_client_id()
{
	for (int i = 0; i < clients.size(); i++) {
		lock_guard<mutex> m(clients[i].state_mtx);
		if (clients[i].state == FREE) return i;
	}
	return -1;
}

void process_packet(int client_id, char* packet)
{
	switch (packet[1]) {
	case P_CS_MOVE:
	{
		SESSION* moved_client = &clients[client_id];
		CS_MOVE_PACKET* recv_packet = reinterpret_cast<CS_MOVE_PACKET*>(packet);
		memcpy(&moved_client->player.key_input, &recv_packet->ks, sizeof(recv_packet->ks));
		moved_client->player.key_check();
		moved_client->prev_time = recv_packet->time;

		unordered_set<int> new_view_list;	//새로 업데이트 할 뷰 리스트
		moved_client->view_list_mtx.lock();
		unordered_set<int> old_view_list = moved_client->view_list;	//이전 뷰 리스트 복사
		moved_client->view_list_mtx.unlock();
		
		int num_clients = 0;
		for (auto& client : clients) {	
			if (++num_clients > connected_players.load()) break;
			
			{
				lock_guard<mutex> m(clients[client_id].state_mtx);
				if (client.state != INGAME) continue;
			}
			if (client.client_id == client_id) continue;
			
			if (in_eyesight(client_id, client.client_id)) {	//현재 시야 안에 있는 클라이언트
				new_view_list.insert(client.client_id);			//new list 채우기
			}
		}
		for (auto& old_one : old_view_list) {
			if (new_view_list.count(old_one) == 0) {			//시야에서 사라진 플레이어
				moved_client->send_out_packet(old_one);		//움직인 플레이어한테 목록에서 사라진 플레이어 삭제 패킷 보냄
				clients[old_one].send_out_packet(client_id);		//시야에서 사라진 플레이어에게 움직인 플레이어 삭제 패킷 보냄

				clients[old_one].erase_view_list(client_id);	//시야에서 사라진 플레이어의 뷰 리스트에서 움직인 플레이어 삭제
			}	
		}
		for (auto& new_one : new_view_list) {				//새로운 뷰 리스트에는 다 움직임 패킷
			moved_client->send_move_packet(new_one);
			clients[new_one].send_move_packet(client_id);
			
			if(old_view_list.count(new_one) == 0){			//새로 시야에 들어온 플레이어
				clients[new_one].insert_view_list(client_id);//시야에 들어온 플레이어의 뷰 리스트에 움직인 플레이어 추가
			}
		}
		
		moved_client->view_list_mtx.lock();
		moved_client->view_list = new_view_list;					//새로운 뷰 리스트로 갈아치움
		moved_client->view_list_mtx.unlock();

		moved_client->send_move_packet(client_id);					//본인 움직임 보냄
		break;
	}
	case P_CS_LOGIN:
	{
		SESSION* new_client = &clients[client_id];
		CS_LOGIN_PACKET* recv_packet = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
		new_client->send_login_packet();									//새로온 애한테 로그인 됐다고 전송
		{
			lock_guard<mutex> m{ clients[client_id].state_mtx };
			clients[client_id].state = INGAME;
		}
		int num_clients = 0;
		for (auto& old_client : clients) {	
			if (++num_clients > connected_players.load()) break;
			
			{
				lock_guard<mutex> m(clients[client_id].state_mtx);
				if (old_client.state != INGAME) continue;
			}
			if (client_id == old_client.client_id) continue;
			if (in_eyesight(client_id, old_client.client_id)) {
				old_client.insert_view_list(client_id);//시야 안에 들어온 클라의 View list에 새로온 놈 추가
				new_client->insert_view_list(old_client.client_id);//새로온 놈의 viewlist에 시야 안에 들어온 클라 추가

				new_client->send_move_packet(old_client.client_id);			//새로 들어온 클라에게 시야 안의 기존 애들 위치 전송
				old_client.send_move_packet(client_id);						//시야 안에 클라한테 새로온 애 위치 전송
			}
		}
		break;
	}
	default: cout << "Unknown Packet Type" << endl; break;
	}
}

void work_thread(HANDLE h_iocp)
{
	while (true) {
		DWORD num_bytes{};
		ULONG_PTR key;
		WSAOVERLAPPED* over = nullptr;
		//완료된 상태를 가져옴
		BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);
		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);
		//cout << "ID: " << key << " TYPE: " << ex_over->completion_type << " Byte:" << num_bytes << endl;
		if (FALSE == ret) {
			//cout << "GQCS Error on client[" << key << "] " << "COMP TYPE: " << ex_over->completion_type << "\n";
			if (ex_over->completion_type == OP_ACCEPT) cout << "Accept Error";
			else {
				if (ex_over->completion_type == OP_SEND) {
					//cout << "Delete ex over" << endl;
					delete ex_over;
				}
				disconnect(static_cast<int>(key));
				continue;
			}
		}

		if (0 == num_bytes && ex_over->completion_type != OP_ACCEPT) {
			//cout << "GQCS Error on client[" << key << "] " << "COMP TYPE: " << ex_over->completion_type << "\n";
			if (ex_over->completion_type == OP_SEND) {
				//cout << "Delete ex over" << endl;
				delete ex_over;
			}
			disconnect(static_cast<int>(key));
			continue;
		}

		switch (ex_over->completion_type) 
		{
		case OP_SEND: 
		{
			//printf("%4d delete %p\n\n", key, ex_over);
			delete ex_over;
			//_CrtDumpMemoryLeaks();
			break;
		}
		case OP_RECV:
		{
			int data_to_proccess = num_bytes + clients[key].remain_data;
			char* packet = ex_over->data;
			while (data_to_proccess > 0) {
				int packet_size = packet[0];
				if (packet_size <= data_to_proccess) {
					process_packet(static_cast<int>(key), packet);
					packet += packet_size;
					data_to_proccess -= packet_size;
				}
				else break;
			}
			clients[key].remain_data = data_to_proccess;
			if (data_to_proccess > 0) {
				memcpy(ex_over->data, packet, data_to_proccess);
			}
			clients[key].do_recv();
			break;
		}
		case OP_ACCEPT: 
		{
			int client_id = get_new_client_id();
			if (client_id != -1) {
				{
					lock_guard<mutex> m(clients[client_id].state_mtx);
					clients[client_id].state = ALLOC;
				}
				clients[client_id].client_id = client_id;
				clients[client_id].socket = global_client_socket;
				clients[client_id].remain_data = 0;
				clients[client_id].view_list_mtx.lock();
				clients[client_id].view_list.clear();
				clients[client_id].view_list_mtx.unlock();

				CreateIoCompletionPort(reinterpret_cast<HANDLE>(global_client_socket), h_iocp, client_id, 0);
				clients[client_id].do_recv();
				global_client_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			}
			else {
				cout << "Max user exceeded.\n";
			}
			ZeroMemory(&global_accept_over.over, sizeof(global_accept_over.over));
			int addr_size = sizeof(SOCKADDR_IN);
			AcceptEx(global_server_socket, global_client_socket, global_accept_over.data, 0, addr_size + 16, addr_size + 16, 0, &global_accept_over.over);
			break;
		}
		default: cout << "Unknown Completion Type" << endl; break;
		}
	}
}

int main()
{
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	global_server_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;

	bind(global_server_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	listen(global_server_socket, SOMAXCONN);

	SOCKADDR_IN client_addr;
	int addr_size = sizeof(client_addr);
	//IOCP 생성 (마지막 인자 0은 코어 개수만큼 사용)
	HANDLE h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	//먼저 IOCP 생성한걸 ExistingCompletionPort에 넣어줌. key는 임의로 아무거나. 마지막 것은 무시
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(global_server_socket), h_iocp, 9999, 0);
	global_client_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	global_accept_over.completion_type = OP_ACCEPT;
	AcceptEx(global_server_socket, global_client_socket, global_accept_over.data, 0, addr_size + 16, addr_size + 16, 0, &global_accept_over.over);

	vector <thread> worker_threads;
	int num_threads = thread::hardware_concurrency();
	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(work_thread, h_iocp);
	for (auto& th : worker_threads)
		th.join();

	closesocket(global_server_socket);
	WSACleanup();
}

