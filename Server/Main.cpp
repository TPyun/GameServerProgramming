#include <unordered_map>
#include <random>
#include <thread>
#include "Player.h"
using namespace std;

const short SERVER_PORT = 9000;
const int BUFSIZE = 4000;
unsigned short MAP_SIZE = 400;



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
//};
//
//unordered_map <int, SESSION> clients;
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
//			SC_MOVE_PACKET send_packet;
//			if (client_id == client.first)
//				send_packet.client_id = 0;
//			else
//				send_packet.client_id = client_id;
//			send_packet.position = this_client->player.position;
//			if (client.second.do_send((char*)&send_packet)) return;
//		}
//		break;
//	}
//	case CS_LOGIN:
//	{
//		SESSION* this_client = &clients[client_id];
//		CS_LOGIN_PACKET* recv_packet = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
//		for (auto& client : clients) {	//새로 들어온 클라에게 기존 애들 위치 전송
//			SC_MOVE_PACKET packet_for_noob;
//			if (client_id == client.first)
//				packet_for_noob.client_id = 0;
//			else
//				packet_for_noob.client_id = client.first;
//			packet_for_noob.position = client.second.player.position;
//			if (this_client->do_send((char*) & packet_for_noob)) return;
//		}
//		for (auto& client : clients) {	//모든 클라한테 새로온 애 위치 전송
//			SC_MOVE_PACKET packet_for_old_players;
//			if (client_id == client.first)
//				continue;
//			packet_for_old_players.client_id = client_id;
//			packet_for_old_players.position = this_client->player.position;
//			if (client.second.do_send((char*)&packet_for_old_players)) return;
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
//	Player player{ randomly_spawn_player(), client_id };
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













//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//ICOP Multi Thiread 
void delete_session(int);
TI randomly_spawn_player();
void disconnect(int);
enum COMPLETION_TYPE { OP_ACCEPT, OP_RECV, OP_SEND };

class OVER_EXP {
public:
	WSAOVERLAPPED over;	//클래스의 맨 첫번째에 있어야만 함. 이거를 가지고 클래스 포인터 위치 찾을거임
	int root_client_id;
	WSABUF wsa_buf;
	char data[BUFSIZE];
	COMPLETION_TYPE completion_type;

	OVER_EXP()				//Recv
	{
		ZeroMemory(data, sizeof(data));
		wsa_buf.buf = data;
		wsa_buf.len = BUFSIZE;
		completion_type = OP_RECV;
	}

	OVER_EXP(char* packet)	//Send
	{
		ZeroMemory(&over, sizeof(over));
		wsa_buf.buf = packet;
		wsa_buf.len = packet[0];
		completion_type = OP_SEND;
	}

	~OVER_EXP() {}
};

OVER_EXP global_accept_over;
SOCKET global_client_socket;
SOCKET global_server_socket;

class SESSION {
	OVER_EXP recv_over;

private:
	SOCKET socket;
	int client_id{};

public:
	int	remain_data{};
	Player player{ randomly_spawn_player(), client_id };

	SESSION() {
		cout << "Unexpected Constructor Call Error!\n";
		exit(-1);
	}

	SESSION(int id, SOCKET s) : client_id(id), socket(s) {

	}

	~SESSION() {
		closesocket(socket);
	}

	bool do_recv() {
		cout << client_id << " recv\n";
		DWORD recv_flag = 0;
		memset(&recv_over.over, 0, sizeof(recv_over.over));
		recv_over.wsa_buf.len = BUFSIZE - remain_data;
		recv_over.wsa_buf.buf = recv_over.data + remain_data;
		int retval = WSARecv(socket, &recv_over.wsa_buf, 1, 0, &recv_flag, &recv_over.over, 0);
		if (retval == 0 && WSAGetLastError() != WSA_IO_PENDING) {
			cout << "WSARecv() failed with error " << WSAGetLastError() << endl;
			disconnect(client_id);
			return 1;
		}
		return 0;
	}

	bool do_send(void* packet) {
		cout << client_id << " send\n";
		OVER_EXP* send_over = new OVER_EXP{ reinterpret_cast<char*>(packet) };

		int retval = WSASend(socket, &send_over->wsa_buf, 1, 0, 0, &send_over->over, 0);
		cout << retval << endl;

		if (retval == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
			cout << "WSASend() failed with error " << WSAGetLastError() << endl;
			disconnect(client_id);
			delete send_over;
			return 1;
		}
		return 0;
	}
	void send_login_packet();
	void send_move_packet(int);
	void send_out_packet(int);
};

unordered_map <int, SESSION> clients;

void SESSION::send_login_packet()
{
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
	do_send(&packet);
}

void SESSION::send_out_packet(int client_id)
{
	SC_OUT_PACKET packet;
	packet.client_id = client_id;
	do_send(&packet);
}

void delete_session(int client_id)
{
	cout << "DELETE SESSION: " << client_id << endl;
	clients.erase(client_id);
}

void disconnect(int client_id)
{
	cout << "ERROR HANDLING: " << client_id << endl;
	for (auto& client : clients) {
		if (client.first == client_id) continue;
		SC_OUT_PACKET out_packet;
		out_packet.client_id = client_id;
		client.second.do_send(&out_packet);
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

int get_new_client_id()
{
	for (int i = 1; ; i++) {
		if (clients.find(i) == clients.end()) return i;
	}
	return -1;
}

void process_packet(int client_id, char* packet)
{
	switch (packet[1]) {
	case CS_MOVE:
	{
		SESSION* this_client = &clients[client_id];
		CS_MOVE_PACKET* recv_packet = reinterpret_cast<CS_MOVE_PACKET*>(packet);
		memcpy(&this_client->player.key_input, &recv_packet->ks, sizeof(recv_packet->ks));
		this_client->player.key_check();
		for (auto& client : clients) {	//모든 클라한테 데이터 전송
			client.second.send_move_packet(client_id);
		}
		break;
	}
	case CS_LOGIN:
	{
		SESSION* this_client = &clients[client_id];
		CS_LOGIN_PACKET* recv_packet = reinterpret_cast<CS_LOGIN_PACKET*>(packet);

		this_client->send_login_packet();
		for (auto& client : clients) {	//새로 들어온 클라에게 기존 애들 위치 전송
			if (client_id == client.first)
				continue;
			this_client->send_move_packet(client.first);
		}
		for (auto& client : clients) {	//모든 클라한테 새로온 애 위치 전송
			if (client_id == client.first)
				continue;
			client.second.send_move_packet(client_id);
		}
		break;
	}
	}
}

void work_thread(HANDLE h_iocp)
{
	for (;;) {
		DWORD num_bytes;
		ULONG_PTR key;
		WSAOVERLAPPED* over = nullptr;
		BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);
		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);
		if (FALSE == ret) {
			if (ex_over->completion_type == OP_ACCEPT) cout << "Accept Error";
			else {
				cout << "GQCS Error on client[" << key << "]\n";
				disconnect(static_cast<int>(key));
				if (ex_over->completion_type == OP_SEND) delete ex_over;
				continue;
			}
		}

		switch (ex_over->completion_type) {
		case OP_ACCEPT: {
			int client_id = get_new_client_id();
			if (client_id != -1) {
				clients.try_emplace(client_id, client_id, global_client_socket);

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
		case OP_RECV: {
			int data_to_proccess = num_bytes + clients[key].remain_data;
			char* p = ex_over->data;
			while (data_to_proccess > 0) {
				int packet_size = p[0];
				if (packet_size <= data_to_proccess) {
					process_packet(static_cast<int>(key), p);
					p += packet_size;
					data_to_proccess -= packet_size;
				}
				else break;
			}
			clients[key].remain_data = data_to_proccess;
			if (data_to_proccess > 0) {
				memcpy(ex_over->data, p, data_to_proccess);
			}
			clients[key].do_recv();
			break;
		}
		case OP_SEND:
			delete ex_over;
			break;
		}
	}
}

int main()
{
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	global_server_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(global_server_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	listen(global_server_socket, SOMAXCONN);
	INT addr_size = sizeof(server_addr);
	cout << "Port: " << SERVER_PORT << endl;

	HANDLE h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(global_server_socket), h_iocp, 9999, 0);
	global_client_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	global_accept_over.completion_type = OP_ACCEPT;
	AcceptEx(global_server_socket, global_client_socket, global_accept_over.data, 0, addr_size + 16, addr_size + 16, 0, &global_accept_over.over);

	vector <thread> worker_threads;
	int num_threads = std::thread::hardware_concurrency();
	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(work_thread, h_iocp);
	for (auto& th : worker_threads)
		th.join();

	clients.clear();
	closesocket(global_server_socket);
	WSACleanup();
}

