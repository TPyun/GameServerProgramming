//////서버
////
//////#include <iostream>
//////#include <WS2tcpip.h>
//////#pragma comment(lib, "WS2_32.lib")
//////using namespace std;
//////constexpr int PORT_NUM = 3500;
//////constexpr int BUF_SIZE = 200;
//////SOCKET client;
//////WSAOVERLAPPED c_over;
//////WSABUF c_wsabuf[1];
//////CHAR c_mess[BUF_SIZE];
//////
//////void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags);
//////void do_recv()
//////{
//////	c_wsabuf[0].buf = c_mess;
//////	c_wsabuf[0].len = BUF_SIZE;
//////	DWORD recv_flag = 0;
//////	memset(&c_over, 0, sizeof(c_over));
//////	WSARecv(client, c_wsabuf, 1, 0, &recv_flag, &c_over, recv_callback);
//////}
//////void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags)
//////{
//////	do_recv();
//////}
//////void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags)
//////{
//////	if (0 == num_bytes) return;
//////	cout << "Client sent: " << c_mess << endl;
//////	c_wsabuf[0].len = num_bytes;
//////	memset(&c_over, 0, sizeof(c_over));
//////	WSASend(client, c_wsabuf, 1, 0, 0, &c_over, send_callback);
//////}
//////
//////int main()
//////{
//////	WSADATA WSAData;
//////	WSAStartup(MAKEWORD(2, 2), &WSAData);
//////	SOCKET server = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
//////	SOCKADDR_IN server_addr;
//////	memset(&server_addr, 0, sizeof(server_addr));
//////	server_addr.sin_family = AF_INET;
//////	server_addr.sin_port = htons(PORT_NUM);
//////	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
//////	bind(server, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
//////	listen(server, SOMAXCONN);
//////	SOCKADDR_IN cl_addr;
//////	int addr_size = sizeof(cl_addr);
//////	client = WSAAccept(server, reinterpret_cast<sockaddr*>(&cl_addr), &addr_size, NULL, NULL);
//////	do_recv();
//////	while (true) 
//////		SleepEx(100, true);
//////	closesocket(server);
//////	WSACleanup();
//////}
////
////#include <iostream>
////#include <WS2tcpip.h>
////#include <unordered_map>
////using namespace std;
////#pragma comment (lib, "WS2_32.LIB")
////
////const short SERVER_PORT = 4000;
////const int BUFSIZE = 256;
////
////void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED send_over, DWORD recv_flag);
////void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED recv_over, DWORD recv_flag);
////
////class EXP_OVER {
////public:
////	WSAOVERLAPPED wsa_over;
////	unsigned long long s_id;
////	WSABUF wsa_buf;
////	char send_msg[BUFSIZE];
////public:
////	EXP_OVER(unsigned long long s_id, char num_bytes, const char* mess) : s_id(s_id)
////	{
////		ZeroMemory(&wsa_over, sizeof(wsa_over));
////		wsa_buf.buf = send_msg;
////		wsa_buf.len = num_bytes + 2;
////		memcpy(send_msg + 2, mess, num_bytes);
////		send_msg[0] = num_bytes + 2;
////		send_msg[1] = static_cast<unsigned long long>(s_id);
////	}
////	~EXP_OVER() {}
////};
////
////class SESSION {
////private:
////	unsigned long long _id;
////	WSABUF _recv_wsabuf;
////	WSABUF _send_wsabuf;
////	WSAOVERLAPPED _recv_over;
////	SOCKET _socket;
////public:
////	char _recv_buf[BUFSIZE];
////	SESSION() {
////		cout << "Unexpected Constructor Call Error!\n";
////		exit(-1);
////	}
////	SESSION(int id, SOCKET s) : _id(id), _socket(s) {
////		_recv_wsabuf.buf = _recv_buf; _recv_wsabuf.len = BUFSIZE;
////		_send_wsabuf.buf = _recv_buf; _send_wsabuf.len = 0;
////	}
////	~SESSION() { closesocket(_socket); }
////	void do_recv() {
////		DWORD recv_flag = 0;
////		ZeroMemory(&_recv_over, sizeof(_recv_over));
////		_recv_over.hEvent = reinterpret_cast<HANDLE>(_id);
////		WSARecv(_socket, &_recv_wsabuf, 1, 0, &recv_flag, &_recv_over, recv_callback);
////	}
////	void do_send(unsigned long long sender_id, int num_bytes, const char* buff){
////		EXP_OVER* send_over = new EXP_OVER(sender_id, num_bytes, buff);
////		//ZeroMemory(&_recv_over, sizeof(_recv_over));
////		//_recv_over.hEvent = reinterpret_cast<HANDLE>(_id);
////		//_send_wsabuf.len = num_bytes;
////		WSASend(_socket, &send_over->wsa_buf, 1, 0, 0, &send_over->wsa_over, send_callback);
////	}
////};
////
////unordered_map <unsigned long long, SESSION> clients;
////
////void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED send_over, DWORD recv_flag)
////{
////	/*unsigned long long c_id = reinterpret_cast<unsigned long long>(send_over->hEvent);
////	clients[c_id].do_recv();*/
////	delete reinterpret_cast<EXP_OVER*>(send_over);
////}
////void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED recv_over, DWORD recv_flag)
////{
////	unsigned long long c_id = reinterpret_cast<unsigned long long>(recv_over->hEvent);
////	cout << "Client" << c_id << " Sent[" << num_bytes << "bytes] : " << clients[c_id]._recv_buf << endl;
////	for (auto& client : clients) {
////		client.second.do_send(c_id, num_bytes, clients[c_id]._recv_buf);
////		//clients[c_id].do_send(num_bytes, client._recv_buf)
////	}
////	clients[c_id].do_recv();
////}
////int main()
////{
////	WSADATA WSAData;
////	WSAStartup(MAKEWORD(2, 2), &WSAData);
////	SOCKET s_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
////	SOCKADDR_IN server_addr;
////	ZeroMemory(&server_addr, sizeof(server_addr));
////	server_addr.sin_family = AF_INET;
////	server_addr.sin_port = htons(SERVER_PORT);
////	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
////	bind(s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
////	listen(s_socket, SOMAXCONN);
////	INT addr_size = sizeof(server_addr);
////	for (;;) {
////		SOCKET c_socket = WSAAccept(s_socket, reinterpret_cast<sockaddr*>(&server_addr), &addr_size, 0, 0);
////		int i = 1;
////		while (1) {
////			clients.try_emplace(i, i, c_socket);
////			clients[i].do_recv();
////			cout << "Client" << i << " Connected!\n";
////			i++;
////		}
////		
////	}
////	clients.clear();
////	closesocket(s_socket);
////	WSACleanup();
////}
//
//#include <iostream>
//#include <vector>
//#include <thread>
//#include <mutex>
//#include <array>
//#include <WS2tcpip.h>
//#include <MSWSock.h>
//#include "../protocol.h"
//
//#pragma comment(lib, "WS2_32.lib")
//#pragma comment(lib, "MSWSock.lib")
//using namespace std;
//constexpr int MAX_USER = 10;
//
//enum COMP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND };
//enum CL_STATE{STATE_FREE, STATE_ALLOC, STATE_INGAME};
//
//class OVER_EXP {
//public:
//	WSAOVERLAPPED _over;
//	WSABUF _wsabuf;
//	char _send_buf[BUF_SIZE];
//	COMP_TYPE _comp_type;
//	SOCKET accept_socket;
//	OVER_EXP()
//	{
//		_wsabuf.len = BUF_SIZE;
//		_wsabuf.buf = _send_buf;
//		_comp_type = OP_RECV;
//		ZeroMemory(&_over, sizeof(_over));
//	}
//	OVER_EXP(unsigned char* packet)
//	{
//		_wsabuf.len = packet[0];
//		_wsabuf.buf = _send_buf;
//		ZeroMemory(&_over, sizeof(_over));
//		_comp_type = OP_SEND;
//		memcpy(_send_buf, packet, packet[0]);
//	}
//};
//
//class SESSION {
//	OVER_EXP _recv_over;
//
//public:
//	CL_STATE _state;
//	mutex _state_lock;
//	bool in_use;		//배열이라서 사용중인지 아닌지 넣음
//	int _id;
//	SOCKET _socket;
//	short	x, y;
//	char	_name[NAME_SIZE];
//
//	int		_prev_remain;
//public:
//	SESSION() : _socket(0), in_use(false)
//	{
//		_id = -1;
//		x = y = 0;
//		_name[0] = 0;
//		_prev_remain = 0;
//	}
//
//	~SESSION() {}
//
//	void do_recv()
//	{
//		DWORD recv_flag = 0;
//		memset(&_recv_over._over, 0, sizeof(_recv_over._over));
//		_recv_over._wsabuf.len = BUF_SIZE - _prev_remain;		//패킷 재조립, 저번에 받은거 합치기
//		_recv_over._wsabuf.buf = _recv_over._send_buf + _prev_remain;
//		WSARecv(_socket, &_recv_over._wsabuf, 1, 0, &recv_flag, &_recv_over._over, 0);
//	}
//
//	void do_send(void* packet)
//	{
//		OVER_EXP* sdata = new OVER_EXP{ reinterpret_cast<unsigned char*>(packet) };
//		WSASend(_socket, &sdata->_wsabuf, 1, 0, 0, &sdata->_over, 0);
//	}
//	void send_login_info_packet()
//	{
//		SC_LOGIN_INFO_PACKET p;
//		p.id = _id;
//		p.size = sizeof(SC_LOGIN_INFO_PACKET);
//		p.type = SC_LOGIN_INFO;
//		p.x = x;
//		p.y = y;
//		do_send(&p);
//	}
//	void send_move_packet(int c_id);
//};
//
//array<SESSION, MAX_USER> clients;
//HANDLE g_h_iocp;
//
//
//void SESSION::send_move_packet(int c_id)
//{
//	SC_MOVE_PLAYER_PACKET p;
//	p.id = c_id;
//	p.size = sizeof(SC_MOVE_PLAYER_PACKET);
//	p.type = SC_MOVE_PLAYER;
//	p.x = clients[c_id].x;
//	p.y = clients[c_id].y;
//	do_send(&p);
//}
//
//int get_new_client_id()
//{
//	for (int i = 0; i < MAX_USER; ++i) {
//		clients[i]._state_lock.lock();
//		if (clients[i]._state == STATE_FREE)
//			return i;
//	}
//		
//	return -1;
//}
//
//void process_packet(int c_id, char* packet)
//{
//	switch (packet[1]) {
//	case CS_LOGIN: {
//		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
//		strcpy_s(clients[c_id]._name, p->name);
//		clients[c_id].send_login_info_packet();
//
//		for (auto& pl : clients) {
//			if (false == pl.in_use) continue;
//			if (pl._id == c_id) continue;
//			SC_ADD_PLAYER_PACKET add_packet;
//			add_packet.id = c_id;
//			strcpy_s(add_packet.name, p->name);
//			add_packet.size = sizeof(add_packet);
//			add_packet.type = SC_ADD_PLAYER;
//			add_packet.x = clients[c_id].x;
//			add_packet.y = clients[c_id].y;
//			pl.do_send(&add_packet);
//		}
//		for (auto& pl : clients) {
//			if (false == pl.in_use) continue;
//			if (pl._id == c_id) continue;
//			SC_ADD_PLAYER_PACKET add_packet;
//			add_packet.id = pl._id;
//			strcpy_s(add_packet.name, pl._name);
//			add_packet.size = sizeof(add_packet);
//			add_packet.type = SC_ADD_PLAYER;
//			add_packet.x = pl.x;
//			add_packet.y = pl.y;
//			clients[c_id].do_send(&add_packet);
//		}
//		break;
//	}
//	case CS_MOVE: {
//		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
//		short x = clients[c_id].x;
//		short y = clients[c_id].y;
//		switch (p->direction) {
//		case 0: if (y > 0) y--; break;
//		case 1: if (y < W_HEIGHT - 1) y++; break;
//		case 2: if (x > 0) x--; break;
//		case 3: if (x < W_WIDTH - 1) x++; break;
//		}
//		clients[c_id].x = x;
//		clients[c_id].y = y;
//		for (auto& pl : clients)
//			if (true == pl.in_use)
//				pl.send_move_packet(c_id);
//		break;
//	}
//	}
//}
//
//void disconnect(int c_id)
//{
//	for (auto& pl : clients) {
//		if (pl.in_use == false) continue;
//		if (pl._id == c_id) continue;
//		SC_REMOVE_PLAYER_PACKET p;
//		p.id = c_id;
//		p.size = sizeof(p);
//		p.type = SC_REMOVE_PLAYER;
//		pl.do_send(&p);
//	}
//	closesocket(clients[c_id]._socket);
//	clients[c_id].in_use = false;
//}
//
//void work()
//{
//	while (true) {
//		DWORD num_bytes;
//		ULONG_PTR key{};
//		WSAOVERLAPPED* over = nullptr;
//		BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);
//
//		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);
//		if (FALSE == ret) {
//			if (ex_over->_comp_type == OP_ACCEPT) {
//				cout << "Accept Error";
//				exit(-1);
//			}
//			else {
//				cout << "GQCS Error on client[" << key << "]\n";
//				disconnect(static_cast<int>(key));
//				if (ex_over->_comp_type == OP_SEND) delete ex_over;
//				continue;
//			}
//		}
//	}
//}
//
//int main()
//{
//
//	WSADATA WSAData;
//	WSAStartup(MAKEWORD(2, 2), &WSAData);
//	SOCKET server = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
//	SOCKADDR_IN server_addr;
//	memset(&server_addr, 0, sizeof(server_addr));
//	server_addr.sin_family = AF_INET;
//	server_addr.sin_port = htons(PORT_NUM);
//	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
//	bind(server, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
//	listen(server, SOMAXCONN);
//	SOCKADDR_IN cl_addr;
//	int addr_size = sizeof(cl_addr);
//	
//	//iocp객체 생성
//	g_h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
//	CreateIoCompletionPort(reinterpret_cast<HANDLE>(server), g_h_iocp, 9999, 0);
//	SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
//	OVER_EXP a_over;
//	a_over._comp_type = OP_ACCEPT;
//	a_over.accept_socket = c_socket;
//	AcceptEx(server, c_socket, a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &a_over._over);
//
//
//
//	
//	vector<thread>worker_threads;
//	int num_cores = thread::hardware_concurrency();
//	cout << "num of cores : " << num_cores << endl;
//	for (int i = 0; i < num_cores; i++) {
//		worker_threads.emplace_back(worker_thread);
//		
//	}
//	for(auto & th: worker_threads)
//		th.join();
//	
//	
//
//
//
//	
//	switch (ex_over->_comp_type) {
//	case OP_ACCEPT: {
//		int client_id = get_new_client_id();
//		if (client_id != -1) {
//			clients[client_id].in_use = true;
//			clients[client_id].x = 0;
//			clients[client_id].y = 0;
//			clients[client_id]._id = client_id;
//			clients[client_id]._name[0] = 0;
//			clients[client_id]._prev_remain = 0;
//			clients[client_id]._socket = c_socket;
//			CreateIoCompletionPort(reinterpret_cast<HANDLE>(c_socket),h_iocp, client_id, 0);
//			clients[client_id].do_recv();
//		}
//		else {
//			cout << "Max user exceeded.\n";
//			closesocket(c_socket);
//		}
//		SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
//		ZeroMemory(&a_over._over, sizeof(a_over._over));
//		AcceptEx(server, c_socket, a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &a_over._over);
//		break;
//	}
//	case OP_RECV: {
//		int remain_data = num_bytes + clients[key]._prev_remain;
//		char* p = ex_over->_send_buf;
//		while (remain_data > 0) {
//			int packet_size = p[0];
//			if (packet_size <= remain_data) {
//				process_packet(static_cast<int>(key), p);
//				p = p + packet_size;
//				remain_data = remain_data - packet_size;
//			}
//			else break;
//		}
//		clients[key]._prev_remain = remain_data;
//		if (remain_data > 0) {
//			memcpy(ex_over->_send_buf, p, remain_data);
//		}
//		clients[key].do_recv();
//		break;
//	}
//	case OP_SEND:
//		delete ex_over;
//		break;
//	}
//	
//	closesocket(server);
//	WSACleanup();
//}
