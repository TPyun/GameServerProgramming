//시야처리 없는 버전									아래에 시야처리 된 코드 있습니다.
// 
// 
// 
//#include <iostream>
//#include <array>
//#include <WS2tcpip.h>
//#include <MSWSock.h>
//#include <thread>
//#include <vector>
//#include <mutex>
//#include <unordered_set>
//#include "..\protocol.h"
//
//#pragma comment(lib, "WS2_32.lib")
//#pragma comment(lib, "MSWSock.lib")
//using namespace std;
//
//enum COMP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND };
//class OVER_EXP {
//public:
//	WSAOVERLAPPED _over;
//	WSABUF _wsabuf;
//	char _send_buf[BUF_SIZE];
//	COMP_TYPE _comp_type;
//	OVER_EXP()
//	{
//		_wsabuf.len = BUF_SIZE;
//		_wsabuf.buf = _send_buf;
//		_comp_type = OP_RECV;
//		ZeroMemory(&_over, sizeof(_over));
//	}
//	OVER_EXP(char* packet)
//	{
//		_wsabuf.len = packet[0];
//		_wsabuf.buf = _send_buf;
//		ZeroMemory(&_over, sizeof(_over));
//		_comp_type = OP_SEND;
//		memcpy(_send_buf, packet, packet[0]);
//	}
//};
//
//enum S_STATE { ST_FREE, ST_ALLOC, ST_INGAME };
//class SESSION {
//	OVER_EXP _recv_over;
//
//public:
//	mutex _s_lock;
//	S_STATE _state;
//	int _id;
//	SOCKET _socket;
//	short	x, y;
//	char	_name[NAME_SIZE];
//	int		_prev_remain;
//	int		_last_move_time;
//public:
//	SESSION()
//	{
//		_id = -1;
//		_socket = 0;
//		x = y = 0;
//		_name[0] = 0;
//		_state = ST_FREE;
//		_prev_remain = 0;
//	}
//
//	~SESSION() {}
//
//	void do_recv()
//	{
//		DWORD recv_flag = 0;
//		memset(&_recv_over._over, 0, sizeof(_recv_over._over));
//		_recv_over._wsabuf.len = BUF_SIZE - _prev_remain;
//		_recv_over._wsabuf.buf = _recv_over._send_buf + _prev_remain;
//		WSARecv(_socket, &_recv_over._wsabuf, 1, 0, &recv_flag,
//			&_recv_over._over, 0);
//	}
//
//	void do_send(void* packet)
//	{
//		OVER_EXP* sdata = new OVER_EXP{ reinterpret_cast<char*>(packet) };
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
//	void send_add_player_packet(int c_id);
//	void send_remove_player_packet(int c_id)
//	{
//		SC_REMOVE_PLAYER_PACKET p;
//		p.id = c_id;
//		p.size = sizeof(p);
//		p.type = SC_REMOVE_PLAYER;
//		do_send(&p);
//	}
//};
//
//array<SESSION, MAX_USER> clients;
//
//SOCKET g_s_socket, g_c_socket;
//OVER_EXP g_a_over;
//
//void SESSION::send_move_packet(int c_id)
//{
//	SC_MOVE_PLAYER_PACKET p;
//	p.id = c_id;
//	p.size = sizeof(SC_MOVE_PLAYER_PACKET);
//	p.type = SC_MOVE_PLAYER;
//	p.x = clients[c_id].x;
//	p.y = clients[c_id].y;
//	p.move_time = clients[c_id]._last_move_time;
//	do_send(&p);
//}
//
//void SESSION::send_add_player_packet(int c_id)
//{
//	SC_ADD_PLAYER_PACKET add_packet;
//	add_packet.id = c_id;
//	strcpy_s(add_packet.name, clients[c_id]._name);
//	add_packet.size = sizeof(add_packet);
//	add_packet.type = SC_ADD_PLAYER;
//	add_packet.x = clients[c_id].x;
//	add_packet.y = clients[c_id].y;
//	do_send(&add_packet);
//}
//
//int get_new_client_id()
//{
//	for (int i = 0; i < MAX_USER; ++i) {
//		lock_guard <mutex> ll{ clients[i]._s_lock };
//		if (clients[i]._state == ST_FREE)
//			return i;
//	}
//	return -1;
//}
//
//void process_packet(int c_id, char* packet)
//{
//	switch (packet[1]) {
//	case CS_LOGIN: {
//		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
//		strcpy_s(clients[c_id]._name, p->name);
//		clients[c_id].x = rand() % W_WIDTH;
//		clients[c_id].y = rand() % W_HEIGHT;
//		clients[c_id].send_login_info_packet();
//		{
//			lock_guard<mutex> ll{ clients[c_id]._s_lock };
//			clients[c_id]._state = ST_INGAME;
//		}
//		for (auto& pl : clients) {
//			{
//				lock_guard<mutex> ll(pl._s_lock);
//				if (ST_INGAME != pl._state) continue;
//			}
//			if (pl._id == c_id) continue;
//			pl.send_add_player_packet(c_id);
//			clients[c_id].send_add_player_packet(pl._id);
//		}
//		break;
//	}
//	case CS_MOVE: {
//		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
//		clients[c_id]._last_move_time = p->move_time;
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
//
//		for (auto& cl : clients) {
//			if (cl._state != ST_INGAME) continue;
//			cl.send_move_packet(c_id);
//
//		}
//	}
//	}
//}
//
//void disconnect(int c_id)
//{
//	for (auto& pl : clients) {
//		{
//			lock_guard<mutex> ll(pl._s_lock);
//			if (ST_INGAME != pl._state) continue;
//		}
//		if (pl._id == c_id) continue;
//		pl.send_remove_player_packet(c_id);
//	}
//	closesocket(clients[c_id]._socket);
//
//	lock_guard<mutex> ll(clients[c_id]._s_lock);
//	clients[c_id]._state = ST_FREE;
//}
//
//void worker_thread(HANDLE h_iocp)
//{
//	while (true) {
//		DWORD num_bytes;
//		ULONG_PTR key;
//		WSAOVERLAPPED* over = nullptr;
//		BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);
//		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);
//		if (FALSE == ret) {
//			if (ex_over->_comp_type == OP_ACCEPT) cout << "Accept Error";
//			else {
//				cout << "GQCS Error on client[" << key << "]\n";
//				disconnect(static_cast<int>(key));
//				if (ex_over->_comp_type == OP_SEND) delete ex_over;
//				continue;
//			}
//		}
//
//		if ((0 == num_bytes) && ((ex_over->_comp_type == OP_RECV) || (ex_over->_comp_type == OP_SEND))) {
//			disconnect(static_cast<int>(key));
//			if (ex_over->_comp_type == OP_SEND) delete ex_over;
//			continue;
//		}
//
//		switch (ex_over->_comp_type) {
//		case OP_ACCEPT: {
//			int client_id = get_new_client_id();
//			if (client_id != -1) {
//				{
//					lock_guard<mutex> ll(clients[client_id]._s_lock);
//					clients[client_id]._state = ST_ALLOC;
//				}
//				clients[client_id].x = 0;
//				clients[client_id].y = 0;
//				clients[client_id]._id = client_id;
//				clients[client_id]._name[0] = 0;
//				clients[client_id]._prev_remain = 0;
//				clients[client_id]._socket = g_c_socket;
//				CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_c_socket),
//					h_iocp, client_id, 0);
//				clients[client_id].do_recv();
//				g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
//			}
//			else {
//				cout << "Max user exceeded.\n";
//			}
//			ZeroMemory(&g_a_over._over, sizeof(g_a_over._over));
//			int addr_size = sizeof(SOCKADDR_IN);
//			AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);
//			break;
//		}
//		case OP_RECV: {
//			int remain_data = num_bytes + clients[key]._prev_remain;
//			char* p = ex_over->_send_buf;
//			while (remain_data > 0) {
//				int packet_size = p[0];
//				if (packet_size <= remain_data) {
//					process_packet(static_cast<int>(key), p);
//					p = p + packet_size;
//					remain_data = remain_data - packet_size;
//				}
//				else break;
//			}
//			clients[key]._prev_remain = remain_data;
//			if (remain_data > 0) {
//				memcpy(ex_over->_send_buf, p, remain_data);
//			}
//			clients[key].do_recv();
//			break;
//		}
//		case OP_SEND:
//			delete ex_over;
//			break;
//		}
//	}
//}
//
//int main()
//{
//	HANDLE h_iocp;
//
//	WSADATA WSAData;
//	WSAStartup(MAKEWORD(2, 2), &WSAData);
//	g_s_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
//	SOCKADDR_IN server_addr;
//	memset(&server_addr, 0, sizeof(server_addr));
//	server_addr.sin_family = AF_INET;
//	server_addr.sin_port = htons(PORT_NUM);
//	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
//	bind(g_s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
//	listen(g_s_socket, SOMAXCONN);
//	SOCKADDR_IN cl_addr;
//	int addr_size = sizeof(cl_addr);
//	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
//	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_s_socket), h_iocp, 9999, 0);
//	g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
//	g_a_over._comp_type = OP_ACCEPT;
//	AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);
//
//	vector <thread> worker_threads;
//	int num_threads = std::thread::hardware_concurrency();
//	for (int i = 0; i < num_threads; ++i)
//		worker_threads.emplace_back(worker_thread, h_iocp);
//	for (auto& th : worker_threads)
//		th.join();
//	closesocket(g_s_socket);
//	WSACleanup();
//}



////시야처리
//#include <iostream>
//#include <array>
//#include <WS2tcpip.h>
//#include <MSWSock.h>
//#include <thread>
//#include <vector>
//#include <mutex>
//#include <unordered_set>
//#include <concurrent_unordered_set.h>
//#include "..\protocol.h"
//
//
//#pragma comment(lib, "WS2_32.lib")
//#pragma comment(lib, "MSWSock.lib")
//using namespace std;
//
//constexpr int VIEW_RANGE = 5;
//
//enum COMP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND };
//class OVER_EXP {
//public:
//	WSAOVERLAPPED _over;
//	WSABUF _wsabuf;
//	char _send_buf[BUF_SIZE];
//	COMP_TYPE _comp_type;
//	OVER_EXP()
//	{
//		_wsabuf.len = BUF_SIZE;
//		_wsabuf.buf = _send_buf;
//		_comp_type = OP_RECV;
//		ZeroMemory(&_over, sizeof(_over));
//	}
//	OVER_EXP(char* packet)
//	{
//		_wsabuf.len = packet[0];
//		_wsabuf.buf = _send_buf;
//		ZeroMemory(&_over, sizeof(_over));
//		_comp_type = OP_SEND;
//		memcpy(_send_buf, packet, packet[0]);
//	}
//};
//
//enum S_STATE { ST_FREE, ST_ALLOC, ST_INGAME };
//class SESSION {
//	OVER_EXP _recv_over;
//
//public:
//	mutex _s_lock;
//	S_STATE _state;
//	int _id;
//	SOCKET _socket;
//	short	x, y;
//	char	_name[NAME_SIZE];
//	int		_prev_remain;
//	int		_last_move_time;
//
//	//concurrency::concurrent_unordered_set<int> _view_list;
//	unordered_set<int> _view_list;
//	mutex _vl;
//public:
//	SESSION()
//	{
//		_id = -1;
//		_socket = 0;
//		x = y = 0;
//		_name[0] = 0;
//		_state = ST_FREE;
//		_prev_remain = 0;
//	}
//
//	~SESSION() {}
//
//	void do_recv()
//	{
//		//cout << "Recv" << endl;
//
//		DWORD recv_flag = 0;
//		memset(&_recv_over._over, 0, sizeof(_recv_over._over));
//		_recv_over._wsabuf.len = BUF_SIZE - _prev_remain;
//		_recv_over._wsabuf.buf = _recv_over._send_buf + _prev_remain;
//		int retval = WSARecv(_socket, &_recv_over._wsabuf, 1, 0, &recv_flag, &_recv_over._over, 0);
//		//cout << retval << endl;
//	}
//
//	void do_send(void* packet)
//	{
//		//cout << "Send" << endl;
//
//		OVER_EXP* sdata = new OVER_EXP{ reinterpret_cast<char*>(packet) };
//		int retval = WSASend(_socket, &sdata->_wsabuf, 1, 0, 0, &sdata->_over, 0);
//		//cout << retval << endl;
//	}
//
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
//
//	void send_move_packet(int c_id);
//
//	void send_add_player_packet(int c_id);
//
//	void send_remove_player_packet(int c_id)
//	{
//		_vl.lock();
//		if (_view_list.count(c_id) == 0) {
//			_vl.unlock();
//			return;
//		}
//		_view_list.erase(c_id);
//		_vl.unlock();
//
//		SC_REMOVE_PLAYER_PACKET p;
//		p.id = c_id;
//		p.size = sizeof(p);
//		p.type = SC_REMOVE_PLAYER;
//		do_send(&p);
//	}
//};
//
//array<SESSION, MAX_USER> clients;
//
//SOCKET g_s_socket, g_c_socket;
//OVER_EXP g_a_over;
//
//void SESSION::send_move_packet(int c_id)
//{
//	_vl.lock();
//	if (_view_list.count(c_id) != 0) {
//		_vl.unlock();
//		SC_MOVE_PLAYER_PACKET p;
//		p.id = c_id;
//		p.size = sizeof(SC_MOVE_PLAYER_PACKET);
//		p.type = SC_MOVE_PLAYER;
//		p.x = clients[c_id].x;
//		p.y = clients[c_id].y;
//		p.move_time = clients[c_id]._last_move_time;
//		do_send(&p);
//	}
//	else {
//		_vl.unlock();
//		send_add_player_packet(c_id);
//	}
//}
//
//void SESSION::send_add_player_packet(int c_id)
//{
//	_vl.lock();
//	if (_view_list.count(c_id) != 0) {
//		_vl.unlock();
//		send_move_packet(c_id);
//		return;
//	}
//	_view_list.insert(c_id);
//	_vl.unlock();
//
//	SC_ADD_PLAYER_PACKET add_packet;
//	add_packet.id = c_id;
//	strcpy_s(add_packet.name, clients[c_id]._name);
//	add_packet.size = sizeof(add_packet);
//	add_packet.type = SC_ADD_PLAYER;
//	add_packet.x = clients[c_id].x;
//	add_packet.y = clients[c_id].y;
//	do_send(&add_packet);
//}
//
//int get_new_client_id()
//{
//	for (int i = 0; i < MAX_USER; ++i) {
//		lock_guard <mutex> ll{ clients[i]._s_lock };
//		if (clients[i]._state == ST_FREE)
//			return i;
//	}
//	return -1;
//}
//
//bool can_see(int p1, int p2)
//{
//	// return VIEW_RANGE <= SQRT((p1.x - p2.x) ^ 2 + (p1.y - p2.y) ^ 2);
//	if (abs(clients[p1].x - clients[p2].x) > VIEW_RANGE) return false;
//	if (abs(clients[p1].y - clients[p2].y) > VIEW_RANGE) return false;
//	return true;
//}
//
//void process_packet(int c_id, char* packet)
//{
//	switch (packet[1]) {
//	case CS_LOGIN: {
//		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
//		strcpy_s(clients[c_id]._name, p->name);
//		clients[c_id].x = rand() % W_WIDTH;
//		clients[c_id].y = rand() % W_HEIGHT;
//		clients[c_id].send_login_info_packet();
//		{
//			lock_guard<mutex> ll{ clients[c_id]._s_lock };
//			clients[c_id]._state = ST_INGAME;
//		}
//		for (auto& pl : clients) {
//			{
//				lock_guard<mutex> ll(pl._s_lock);
//				if (ST_INGAME != pl._state) continue;
//			}
//			if (pl._id == c_id) continue;
//			if (can_see(c_id, pl._id) == false) continue;
//			pl.send_add_player_packet(c_id);
//			clients[c_id].send_add_player_packet(pl._id);
//		}
//		break;
//	}
//	case CS_MOVE: {
//		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
//		clients[c_id]._last_move_time = p->move_time;
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
//
//		clients[c_id]._vl.lock();
//		auto old_vl = clients[c_id]._view_list;
//		clients[c_id]._vl.unlock();
//
//		unordered_set <int> new_vl;
//		for (auto& cl : clients) {
//			if (cl._state != ST_INGAME) continue;
//			if (cl._id == c_id) continue;
//			if (can_see(cl._id, c_id))	//new vl에 시야처리 새로 해서 추가
//				new_vl.insert(cl._id);
//		}
//		//old와 new 비교
//		for (auto& new_one : new_vl) {
//			//new에 있는 플레이어가 old에 없으면 add player packer보냄
//			if (old_vl.count(new_one) == 0) {
//				clients[new_one].send_add_player_packet(c_id);
//				clients[c_id].send_add_player_packet(new_one);
//			}
//			//전에도 있고 지금도 있으면 move packet
//			else {
//				clients[new_one].send_move_packet(c_id);
//				clients[c_id].send_move_packet(new_one);
//
//			}
//		}
//		for (auto& old_one : old_vl) {
//			//old에 있는 플레이어가 new에 없으면 remove player packet 보냄
//			if (new_vl.count(old_one) == 0) {
//				clients[old_one].send_remove_player_packet(c_id);
//				clients[c_id].send_remove_player_packet(old_one);
//			}
//		}
//		clients[c_id]._vl.lock();
//		clients[c_id]._view_list = new_vl;
//		clients[c_id]._vl.unlock();
//
//
//		/*for (auto& cl : clients) {
//			if (cl._state != ST_INGAME) continue;
//			cl.send_move_packet(c_id);
//
//		}*/
//		clients[c_id].send_move_packet(c_id);
//
//	}
//	}
//}
//
//void disconnect(int c_id)
//{
//	for (auto& pl : clients) {
//		{
//			lock_guard<mutex> ll(pl._s_lock);
//			if (ST_INGAME != pl._state) continue;
//		}
//		if (pl._id == c_id) continue;
//		pl.send_remove_player_packet(c_id);
//	}
//	closesocket(clients[c_id]._socket);
//
//	lock_guard<mutex> ll(clients[c_id]._s_lock);
//	clients[c_id]._state = ST_FREE;
//}
//
//void worker_thread(HANDLE h_iocp)
//{
//	while (true) {
//		DWORD num_bytes;
//		ULONG_PTR key;
//		WSAOVERLAPPED* over = nullptr;
//		BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);
//		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);
//		if (FALSE == ret) {
//			if (ex_over->_comp_type == OP_ACCEPT) cout << "Accept Error";
//			else {
//				cout << "GQCS Error on client[" << key << "]\n";
//				disconnect(static_cast<int>(key));
//				if (ex_over->_comp_type == OP_SEND) delete ex_over;
//				continue;
//			}
//		}
//
//		if ((0 == num_bytes) && ((ex_over->_comp_type == OP_RECV) || (ex_over->_comp_type == OP_SEND))) {
//			disconnect(static_cast<int>(key));
//			if (ex_over->_comp_type == OP_SEND) delete ex_over;
//			continue;
//		}
//
//		switch (ex_over->_comp_type) {
//		case OP_ACCEPT: {
//			int client_id = get_new_client_id();
//			if (client_id != -1) {
//				{
//					lock_guard<mutex> ll(clients[client_id]._s_lock);
//					clients[client_id]._state = ST_ALLOC;
//				}
//				clients[client_id].x = 0;
//				clients[client_id].y = 0;
//				clients[client_id]._id = client_id;
//				clients[client_id]._name[0] = 0;
//				clients[client_id]._prev_remain = 0;
//				clients[client_id]._socket = g_c_socket;
//				CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_c_socket), h_iocp, client_id, 0);
//				clients[client_id].do_recv();
//				g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
//			}
//			else {
//				cout << "Max user exceeded.\n";
//			}
//			ZeroMemory(&g_a_over._over, sizeof(g_a_over._over));
//			int addr_size = sizeof(SOCKADDR_IN);
//			AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);
//			break;
//		}
//		case OP_RECV: {
//			int remain_data = num_bytes + clients[key]._prev_remain;
//			char* p = ex_over->_send_buf;
//			while (remain_data > 0) {
//				int packet_size = p[0];
//				if (packet_size <= remain_data) {
//					process_packet(static_cast<int>(key), p);
//					p = p + packet_size;
//					remain_data = remain_data - packet_size;
//				}
//				else break;
//			}
//			clients[key]._prev_remain = remain_data;
//			if (remain_data > 0) {
//				memcpy(ex_over->_send_buf, p, remain_data);
//			}
//			clients[key].do_recv();
//			break;
//		}
//		case OP_SEND:
//			delete ex_over;
//			break;
//		}
//	}
//}
//
//int main()
//{
//	HANDLE h_iocp;
//
//	WSADATA WSAData;
//	WSAStartup(MAKEWORD(2, 2), &WSAData);
//	g_s_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
//	SOCKADDR_IN server_addr;
//	memset(&server_addr, 0, sizeof(server_addr));
//	server_addr.sin_family = AF_INET;
//	server_addr.sin_port = htons(PORT_NUM);
//	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
//	bind(g_s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
//	listen(g_s_socket, SOMAXCONN);
//	SOCKADDR_IN cl_addr;
//	int addr_size = sizeof(cl_addr);
//	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
//	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_s_socket), h_iocp, 9999, 0);
//	g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
//	g_a_over._comp_type = OP_ACCEPT;
//	AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);
//
//	vector <thread> worker_threads;
//	int num_threads = std::thread::hardware_concurrency();
//	for (int i = 0; i < num_threads; ++i)
//		worker_threads.emplace_back(worker_thread, h_iocp);
//	for (auto& th : worker_threads)
//		th.join();
//	closesocket(g_s_socket);
//	WSACleanup();
//}





//시야처리 버전
#include <iostream>
#include <array>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <vector>
#include <mutex>
#include <unordered_set>
#include <concurrent_unordered_set.h>
#include "..\protocol.h"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")
using namespace std;

constexpr int VIEW_RANGE = 5;
int over_num = 0;

enum COMP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND };
class OVER_EXP {
public:
	WSAOVERLAPPED _over;
	WSABUF _wsabuf;
	char _send_buf[BUF_SIZE];
	COMP_TYPE _comp_type;
	OVER_EXP()
	{
		_wsabuf.len = BUF_SIZE;
		_wsabuf.buf = _send_buf;
		_comp_type = OP_RECV;
		ZeroMemory(&_over, sizeof(_over));
	}
	OVER_EXP(char* packet)
	{
		_wsabuf.len = packet[0];
		_wsabuf.buf = _send_buf;
		ZeroMemory(&_over, sizeof(_over));
		_comp_type = OP_SEND;
		memcpy(_send_buf, packet, packet[0]);
	}
	~OVER_EXP()
	{
	}
};

enum S_STATE { ST_FREE, ST_ALLOC, ST_INGAME };
class SESSION {
	OVER_EXP _recv_over;

public:
	mutex _s_lock;
	S_STATE _state;
	int _id;
	SOCKET _socket;
	short	x, y;
	char	_name[NAME_SIZE];
	int		_prev_remain;
	int		_last_move_time;

	concurrency::concurrent_unordered_set<int> _view_list;
	mutex _vl;
	
public:
	SESSION()
	{
		_id = -1;
		_socket = 0;
		x = y = 0;
		_name[0] = 0;
		_state = ST_FREE;
		_prev_remain = 0;
	}

	~SESSION() {}

	void do_recv()
	{
		//cout << "Recv" << endl;
		
		DWORD recv_flag = 0;
		memset(&_recv_over._over, 0, sizeof(_recv_over._over));
		_recv_over._wsabuf.len = BUF_SIZE - _prev_remain;
		_recv_over._wsabuf.buf = _recv_over._send_buf + _prev_remain;
		int retval = WSARecv(_socket, &_recv_over._wsabuf, 1, 0, &recv_flag,&_recv_over._over, 0);
		if (retval == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
			cout << "WSARecv Error" << endl;
		//cout << retval << endl;
	}

	void do_send(void* packet)
	{
		//cout << "Send" << endl;

		OVER_EXP* sdata = new OVER_EXP{ reinterpret_cast<char*>(packet) };
		int retval = WSASend(_socket, &sdata->_wsabuf, 1, 0, 0, &sdata->_over, 0);
		if (retval == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
			cout << "WSASend Error" << endl;
			delete sdata;
		}
		//cout << retval << endl;
	}

	void send_login_info_packet()
	{
		SC_LOGIN_INFO_PACKET p;
		p.id = _id;
		p.size = sizeof(SC_LOGIN_INFO_PACKET);
		p.type = SC_LOGIN_INFO;
		p.x = x;
		p.y = y;
		do_send(&p);
	}

	void send_move_packet(int c_id);

	void send_add_player_packet(int c_id);

	void send_remove_player_packet(int c_id)
	{
		SC_REMOVE_PLAYER_PACKET p;
		p.id = c_id;
		p.size = sizeof(p);
		p.type = SC_REMOVE_PLAYER;
		do_send(&p);
	}
};

array<SESSION, MAX_USER> clients;

SOCKET g_s_socket, g_c_socket;
OVER_EXP g_a_over;

void SESSION::send_move_packet(int c_id)
{
	SC_MOVE_PLAYER_PACKET p;
	p.id = c_id;
	p.size = sizeof(SC_MOVE_PLAYER_PACKET);
	p.type = SC_MOVE_PLAYER;
	p.x = clients[c_id].x;
	p.y = clients[c_id].y;
	p.move_time = clients[c_id]._last_move_time;
	do_send(&p);
}

void SESSION::send_add_player_packet(int c_id)
{
	SC_ADD_PLAYER_PACKET add_packet;
	add_packet.id = c_id;
	strcpy_s(add_packet.name, clients[c_id]._name);
	add_packet.size = sizeof(add_packet);
	add_packet.type = SC_ADD_PLAYER;
	add_packet.x = clients[c_id].x;
	add_packet.y = clients[c_id].y;
	do_send(&add_packet);
}

int get_new_client_id()
{
	for (int i = 0; i < MAX_USER; ++i) {
		lock_guard <mutex> ll{ clients[i]._s_lock };
		if (clients[i]._state == ST_FREE)
			return i;
	}
	return -1;
}

bool can_see(int p1, int p2)
{
	// return VIEW_RANGE <= SQRT((p1.x - p2.x) ^ 2 + (p1.y - p2.y) ^ 2);
	if (abs(clients[p1].x - clients[p2].x) > VIEW_RANGE) return false;
	if (abs(clients[p1].y - clients[p2].y) > VIEW_RANGE) return false;
	return true;
}

void process_packet(int c_id, char* packet)
{
	switch (packet[1]) {
	case CS_LOGIN: {
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
		strcpy_s(clients[c_id]._name, p->name);
		clients[c_id].x = rand() % W_WIDTH;
		clients[c_id].y = rand() % W_HEIGHT;
		clients[c_id].send_login_info_packet();
		{
			lock_guard<mutex> ll{ clients[c_id]._s_lock };
			clients[c_id]._state = ST_INGAME;
		}
		for (auto& pl : clients) {
			{
				lock_guard<mutex> ll(pl._s_lock);
				if (ST_INGAME != pl._state) continue;
			}
			if (pl._id == c_id) continue;
			if (can_see(c_id, pl._id) == false) continue;
			pl.send_add_player_packet(c_id);
			clients[c_id].send_add_player_packet(pl._id);
		}
		break;
	}
	case CS_MOVE: {
		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
		clients[c_id]._last_move_time = p->move_time;
		short x = clients[c_id].x;
		short y = clients[c_id].y;
		switch (p->direction) {
		case 0: if (y > 0) y--; break;
		case 1: if (y < W_HEIGHT - 1) y++; break;
		case 2: if (x > 0) x--; break;
		case 3: if (x < W_WIDTH - 1) x++; break;
		}
		clients[c_id].x = x;
		clients[c_id].y = y;

		clients[c_id].send_move_packet(c_id);

		clients[c_id]._vl.lock();
		auto old_vl = clients[c_id]._view_list;
		clients[c_id]._vl.unlock();

		unordered_set <int> new_vl;
		for (auto& cl : clients) {
			{
				lock_guard<mutex> ll(cl._s_lock);
				if (ST_INGAME != cl._state) continue;
			}
			if (cl._id == c_id) continue;
			if (can_see(cl._id, c_id))
				new_vl.insert(cl._id);
		}
		//old와 new 비교
		for (auto& new_one : new_vl) {
			//new에 있는 플레이어가 old에 없으면 add player packer보냄
			if (old_vl.count(new_one) == 0) {
				clients[new_one].send_add_player_packet(c_id);
				clients[c_id].send_add_player_packet(new_one);
			}
			//전에도 있고 지금도 있으면 move packet
			else {
				clients[new_one].send_move_packet(c_id);
				clients[c_id].send_move_packet(new_one);

			}
		}
		for (auto& old_one : old_vl) {
			//old에 있는 플레이어가 new에 없으면 remove player packet 보냄
			if (new_vl.count(old_one) == 0) {
				clients[old_one].send_remove_player_packet(c_id);
				clients[c_id].send_remove_player_packet(old_one);
			}
		}
		clients[c_id]._vl.lock();
		clients[c_id]._view_list.clear();
		for (auto& new_one : new_vl)
			clients[c_id]._view_list.insert(new_one);
		clients[c_id]._vl.unlock();

	}
	}
}

void disconnect(int c_id)
{
	for (auto& pl : clients) {
		{
			lock_guard<mutex> ll(pl._s_lock);
			if (ST_INGAME != pl._state) continue;
		}
		if (pl._id == c_id) continue;
		pl.send_remove_player_packet(c_id);
	}
	cout << over_num << endl;
	closesocket(clients[c_id]._socket);

	lock_guard<mutex> ll(clients[c_id]._s_lock);
	clients[c_id]._state = ST_FREE;
}

void worker_thread(HANDLE h_iocp)
{
	while (true) {
		DWORD num_bytes;
		ULONG_PTR key;
		WSAOVERLAPPED* over = nullptr;
		BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);
		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);
		if (FALSE == ret) {
			if (ex_over->_comp_type == OP_ACCEPT) cout << "Accept Error";
			else {
				cout << "GQCS Error on client[" << key << "] " << "COMP TYPE: " << ex_over->_comp_type << "\n";

				disconnect(static_cast<int>(key));
				if (ex_over->_comp_type == OP_SEND) { 
					delete ex_over; 
					cout << "ex delete" << endl;
				}
				continue;
			}
		}

		if ((0 == num_bytes) && ((ex_over->_comp_type == OP_RECV) || (ex_over->_comp_type == OP_SEND))) {
			cout << "GQCS Error on client[" << key << "] " << "COMP TYPE: " << ex_over->_comp_type << "\n";

			disconnect(static_cast<int>(key));
			if (ex_over->_comp_type == OP_SEND){
				delete ex_over;
				cout << "ex delete" << endl;
			}
			continue;
		}

		switch (ex_over->_comp_type) {
		case OP_ACCEPT: {
			int client_id = get_new_client_id();
			if (client_id != -1) {
				{
					lock_guard<mutex> ll(clients[client_id]._s_lock);
					clients[client_id]._state = ST_ALLOC;
				}
				clients[client_id].x = 0;
				clients[client_id].y = 0;
				clients[client_id]._id = client_id;
				clients[client_id]._name[0] = 0;
				clients[client_id]._prev_remain = 0;
				clients[client_id]._socket = g_c_socket;
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_c_socket), h_iocp, client_id, 0);
				clients[client_id].do_recv();
				g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			}
			else {
				cout << "Max user exceeded.\n";
			}
			ZeroMemory(&g_a_over._over, sizeof(g_a_over._over));
			int addr_size = sizeof(SOCKADDR_IN);
			AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);
			break;
		}
		case OP_RECV: {
			int remain_data = num_bytes + clients[key]._prev_remain;
			char* p = ex_over->_send_buf;
			while (remain_data > 0) {
				int packet_size = p[0];
				if (packet_size <= remain_data) {
					process_packet(static_cast<int>(key), p);
					p = p + packet_size;
					remain_data = remain_data - packet_size;
				}
				else break;
			}
			clients[key]._prev_remain = remain_data;
			if (remain_data > 0) {
				memcpy(ex_over->_send_buf, p, remain_data);
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
	HANDLE h_iocp;

	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	g_s_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	bind(g_s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	listen(g_s_socket, SOMAXCONN);
	SOCKADDR_IN cl_addr;
	int addr_size = sizeof(cl_addr);
	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_s_socket), h_iocp, 9999, 0);
	g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_a_over._comp_type = OP_ACCEPT;
	AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);

	vector <thread> worker_threads;
	int num_threads = std::thread::hardware_concurrency();
	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(worker_thread, h_iocp);
	for (auto& th : worker_threads)
		th.join();
	closesocket(g_s_socket);
	WSACleanup();
}
