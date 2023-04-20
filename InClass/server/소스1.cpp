////서버
//#include <iostream>
//#include <array>
//#include<thread>
//#include<vector>
//#include<mutex>
//#include <WS2tcpip.h>
//#include <MSWSock.h>
//#include "../protocol.h"
//
//#pragma comment(lib, "WS2_32.lib")
//#pragma comment(lib, "MSWSock.lib")
//using namespace std;
//constexpr int MAX_USER = 10;
//
//enum class COMP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND };
//enum class CL_STATE { STATE_FREE, STATE_ALLOC, STATE_INGAME };
//HANDLE g_h_iocp;
//SOCKET g_server;
//
//class OVER_EXP {
//public:
//    WSAOVERLAPPED _over;
//    WSABUF _wsabuf;
//    char _send_buf[BUF_SIZE];
//    COMP_TYPE _comp_type;
//    SOCKET accept_socket;
//    OVER_EXP()
//    {
//        _wsabuf.len = BUF_SIZE;
//        _wsabuf.buf = _send_buf;
//        _comp_type = COMP_TYPE::OP_RECV;
//        ZeroMemory(&_over, sizeof(_over));
//    }
//    OVER_EXP(unsigned char* packet)
//    {
//        _wsabuf.len = packet[0];
//        _wsabuf.buf = _send_buf;
//        ZeroMemory(&_over, sizeof(_over));
//        _comp_type = COMP_TYPE::OP_SEND;
//        memcpy(_send_buf, packet, packet[0]);
//    }
//};
//
//class SESSION {
//    OVER_EXP _recv_over;
//
//public:
//    CL_STATE _state;
//    mutex _st_l;
//    //bool in_use;
//    int _id;
//    SOCKET _socket;
//    short   x, y;
//    char   _name[NAME_SIZE];
//
//    int      _prev_remain;
//public:
//    SESSION() : _socket(0), _state(CL_STATE::STATE_FREE)
//    {
//        _id = -1;
//        x = y = 0;
//        _name[0] = 0;
//        _prev_remain = 0;
//    }
//
//    ~SESSION() {}
//
//    void do_recv()
//    {
//        DWORD recv_flag = 0;
//        memset(&_recv_over._over, 0, sizeof(_recv_over._over));
//        _recv_over._wsabuf.len = BUF_SIZE - _prev_remain;
//        _recv_over._wsabuf.buf = _recv_over._send_buf + _prev_remain;
//        WSARecv(_socket, &_recv_over._wsabuf, 1, 0, &recv_flag,
//            &_recv_over._over, 0);
//    }
//
//    void do_send(void* packet)
//    {
//        OVER_EXP* sdata = new OVER_EXP{ reinterpret_cast<unsigned char*>(packet) };
//        WSASend(_socket, &sdata->_wsabuf, 1, 0, 0, &sdata->_over, 0);
//    }
//    void send_login_info_packet()
//    {
//        SC_LOGIN_INFO_PACKET p;
//        p.id = _id;
//        p.size = sizeof(SC_LOGIN_INFO_PACKET);
//        p.type = SC_LOGIN_INFO;
//        p.x = x;
//        p.y = y;
//        do_send(&p);
//    }
//    void send_move_packet(int c_id);
//};
//
//array<SESSION, MAX_USER> clients;
//
//void SESSION::send_move_packet(int c_id)
//{
//    SC_MOVE_PLAYER_PACKET p;
//    p.id = c_id;
//    p.size = sizeof(SC_MOVE_PLAYER_PACKET);
//    p.type = SC_MOVE_PLAYER;
//    p.x = clients[c_id].x;
//    p.y = clients[c_id].y;
//    do_send(&p);
//}
//
//int get_new_client_id()
//{
//    for (int i = 0; i < MAX_USER; ++i) {
//        clients[i]._st_l.lock();
//        if (clients[i]._state == CL_STATE::STATE_FREE)
//        {
//            clients[i]._state = CL_STATE::STATE_ALLOC;
//            clients[i]._st_l.unlock();
//            return i;
//        }
//        clients[i]._st_l.unlock();
//    }
//    return -1;
//}
//
//void process_packet(int c_id, char* packet)
//{
//    switch (packet[1]) {
//    case CS_LOGIN: {
//        CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
//        strcpy_s(clients[c_id]._name, p->name);
//
//        clients[c_id]._st_l.lock();
//        // STATE_INGAME은 _state가 STATE_ALLOC일 경우에만 해주어야 함
//        clients[c_id]._state = CL_STATE::STATE_INGAME;
//        clients[c_id]._st_l.unlock();
//
//        clients[c_id].send_login_info_packet();
//
//        for (auto& pl : clients) {
//            if (CL_STATE::STATE_INGAME != pl._state) continue;
//            if (pl._id == c_id) continue;
//            SC_ADD_PLAYER_PACKET add_packet;
//            add_packet.id = c_id;
//            strcpy_s(add_packet.name, p->name);
//            add_packet.size = sizeof(add_packet);
//            add_packet.type = SC_ADD_PLAYER;
//            add_packet.x = clients[c_id].x;
//            add_packet.y = clients[c_id].y;
//            pl.do_send(&add_packet);
//        }
//        for (auto& pl : clients) {
//            if (CL_STATE::STATE_INGAME != pl._state) continue;
//            if (pl._id == c_id) continue;
//            SC_ADD_PLAYER_PACKET add_packet;
//            add_packet.id = pl._id;
//            strcpy_s(add_packet.name, pl._name);
//            add_packet.size = sizeof(add_packet);
//            add_packet.type = SC_ADD_PLAYER;
//            add_packet.x = pl.x;
//            add_packet.y = pl.y;
//            clients[c_id].do_send(&add_packet);
//        }
//        break;
//    }
//    case CS_MOVE: {
//        CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
//        short x = clients[c_id].x;
//        short y = clients[c_id].y;
//        switch (p->direction) {
//        case 0: if (y > 0) y--; break;
//        case 1: if (y < W_HEIGHT - 1) y++; break;
//        case 2: if (x > 0) x--; break;
//        case 3: if (x < W_WIDTH - 1) x++; break;
//        }
//        clients[c_id].x = x;
//        clients[c_id].y = y;
//        for (auto& pl : clients)
//            if (CL_STATE::STATE_INGAME == pl._state)
//                pl.send_move_packet(c_id);
//        break;
//    }
//    }
//}
//
//void disconnect(int c_id)
//{
//    clients[c_id]._st_l.lock();
//    if (clients[c_id]._state == CL_STATE::STATE_FREE)
//    {
//        clients[c_id]._st_l.unlock();
//        return;
//    }
//    else
//    {
//        clients[c_id]._state = CL_STATE::STATE_FREE;
//        closesocket(clients[c_id]._socket);
//
//        for (auto& pl : clients) {
//            if (pl._state != CL_STATE::STATE_INGAME) continue;
//            if (pl._id == c_id) continue;
//            SC_REMOVE_PLAYER_PACKET p;
//            p.id = c_id;
//            p.size = sizeof(p);
//            p.type = SC_REMOVE_PLAYER;
//            pl.do_send(&p);
//        }
//
//        clients[c_id]._st_l.unlock();
//    }
//
//
//}
//
//void worker_thread()
//{
//    while (true) {
//        DWORD num_bytes;
//        ULONG_PTR key;
//        WSAOVERLAPPED* over = nullptr;
//        BOOL ret = GetQueuedCompletionStatus(g_h_iocp, &num_bytes, &key, &over, INFINITE);
//        OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);
//        if (FALSE == ret) {
//            if (ex_over->_comp_type == COMP_TYPE::OP_ACCEPT) {
//                cout << "Accept Error";
//                DebugBreak();
//                exit(-1);
//            }
//            else {
//                cout << "GQCS Error on client[" << key << "]\n";
//                disconnect(static_cast<int>(key));
//                if (ex_over->_comp_type == COMP_TYPE::OP_SEND) delete ex_over;
//                continue;
//            }
//        }
//
//        switch (ex_over->_comp_type) {
//        case COMP_TYPE::OP_ACCEPT: {
//            SOCKET c_socket = ex_over->accept_socket;
//            int client_id = get_new_client_id();
//            if (client_id != -1) {
//                clients[client_id].x = 0;
//                clients[client_id].y = 0;
//                clients[client_id]._id = client_id;
//                clients[client_id]._name[0] = 0;
//                clients[client_id]._prev_remain = 0;
//                clients[client_id]._socket = c_socket;
//                CreateIoCompletionPort(reinterpret_cast<HANDLE>(c_socket),
//                    g_h_iocp, client_id, 0);
//                clients[client_id].do_recv();
//            }
//            else {
//                cout << "Max user exceeded.\n";
//                closesocket(c_socket);
//            }
//            c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
//            ZeroMemory(&ex_over->_over, sizeof(ex_over->_over));
//            ex_over->accept_socket = c_socket;
//            int addr_size = sizeof(SOCKADDR_IN);
//            AcceptEx(g_server, c_socket, ex_over->_send_buf, 0, addr_size + 16, addr_size + 16, 0, &ex_over->_over);
//            break;
//        }
//        case COMP_TYPE::OP_RECV: {
//            int remain_data = num_bytes + clients[key]._prev_remain;
//            char* p = ex_over->_send_buf;
//            while (remain_data > 0) {
//                int packet_size = p[0];
//                if (packet_size <= remain_data) {
//                    process_packet(static_cast<int>(key), p);
//                    p = p + packet_size;
//                    remain_data = remain_data - packet_size;
//                }
//                else break;
//            }
//            clients[key]._prev_remain = remain_data;
//            if (remain_data > 0) {
//                memcpy(ex_over->_send_buf, p, remain_data);
//            }
//            clients[key].do_recv();
//            break;
//        }
//        case COMP_TYPE::OP_SEND:
//            delete ex_over;
//            break;
//        }
//    }
//}
//
//
//int main()
//{
//
//
//    WSADATA WSAData;
//    WSAStartup(MAKEWORD(2, 2), &WSAData);
//    g_server = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
//    SOCKADDR_IN server_addr;
//    memset(&server_addr, 0, sizeof(server_addr));
//    server_addr.sin_family = AF_INET;
//    server_addr.sin_port = htons(PORT_NUM);
//    server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
//    bind(g_server, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
//    listen(g_server, SOMAXCONN);
//    SOCKADDR_IN cl_addr;
//    int addr_size = sizeof(cl_addr);
//    int client_id = 0;
//
//    g_h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
//    CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_server), g_h_iocp, 9999, 0);
//    SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
//    OVER_EXP* a_over = new OVER_EXP;
//    a_over->_comp_type = COMP_TYPE::OP_ACCEPT;
//    a_over->accept_socket = c_socket;
//    AcceptEx(g_server, c_socket, a_over->_send_buf, 0, addr_size + 16, addr_size + 16, 0, &a_over->_over);
//
//
//    vector<thread> worker_threads;
//    int num_cores = thread::hardware_concurrency();
//    for (int i = 0; i < num_cores; ++i)
//    {
//        worker_threads.emplace_back(worker_thread);
//    }
//    for (auto& th : worker_threads)
//    {
//        th.join();
//    }
//
//    closesocket(g_server);
//    delete a_over;
//    WSACleanup();
//}

//#include <iostream>
//#include <thread>
//#include <Windows.h>
//#include <mutex>
//using namespace std;
//
//
//int* volatile g_data = nullptr;
//
//void receiver()
//{
//	while (nullptr == g_data) {
//	}
//	cout << *g_data << endl;
//}
//
//void sender()
//{
//	g_data = new int{ 100 };
//}
//
//
//int sum = 0;
//atomic<int> victim = 0;
//atomic<bool> p_flag[2] = { false, false };
//
//void p_lock(int th_id)
//{
//	int other = 1 - th_id;
//	p_flag[th_id] = true;
//	victim = th_id;
//	//std::atomic_thread_fence(std::memory_order_seq_cst);
//	while (p_flag[other] && victim == th_id) {
//		// busy wait
//	}
//}
//
//void p_unlock(int th_id)
//{
//	p_flag[th_id] = false;
//}
//
//void worker(int th_id)
//{
//	for (int i = 0; i < 25000000; ++i) {
//		p_lock(th_id);
//		sum += 2;
//		p_unlock(th_id);
//	}
//}
//
////
////volatile bool done = false;
////volatile int* bound;
////int error;
////
////void ThreadFunc1()
////{
////	for (int j = 0; j <= 25000000; ++j) *bound = -(1 + *bound);
////	done = true;
////}
////void ThreadFunc2()
////{
////	while (!done) {
////		int v = *bound;
////		if ((v != 0) && (v != -1)) {
////			error++;
////			printf("%x,", v);
////		}
////	}
////}
//
//int main()
//{
//	/*thread t1(receiver);
//	thread t2(sender);
//
//	t1.join();
//	t2.join();*/
//	
//	thread t1(worker, 0);
//	thread t2(worker, 1);
//	clock_t start = clock();
//	t1.join();
//	t2.join();
//	clock_t end = clock();
//	cout << double(end - start) / CLOCKS_PER_SEC << endl;
//	cout << sum << endl;
//
//	/*int a[32];
//	long long addr = reinterpret_cast<long long>(&a[31]);
//	addr = (addr / 64) * 64;
//	addr = addr - 2;
//	bound = reinterpret_cast<int*>(addr);
//	*bound = 0;*/
//	
//	/*thread t1(ThreadFunc1);
//	thread t2(ThreadFunc2);*/
//	
//	/*t1.join();
//	t2.join();*/
//	
//	//cout << error << endl;
//
//}
//
//
//
////#include <iostream>
////#include <thread>
////#include <chrono>
////#include <vector>
////const auto MAX_THREADS = 256;
////using namespace std;
////using namespace std::chrono;
////atomic<int> sum;
////void thread_func(int num_threads) {
////	for (auto i = 0; i < 50000000 / num_threads; ++i) sum += 2;
////}
////int main() {
////	vector<thread> threads;
////	for (auto i = 1; i <= MAX_THREADS; i *= 2) {
////		sum = 0;
////		threads.clear();
////		auto start = high_resolution_clock::now();
////		for (auto j = 0; j < i; ++j) threads.push_back(thread{ thread_func, i });
////		for (auto& tmp : threads) tmp.join();
////		auto duration = high_resolution_clock::now() - start;
////		cout << i << " Threads, Sum = " << sum;
////		cout << " Duration = " << duration_cast<milliseconds>(duration).count() << " milliseconds\n";
////	}
////}


//
////시야처리 없는 버전
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



//시야처리
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

constexpr int VIEW_RANGE = 4;

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

	//concurrency::concurrent_unordered_set<int> _view_list;
	unordered_set<int> _view_list;
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
		int retval = WSARecv(_socket, &_recv_over._wsabuf, 1, 0, &recv_flag, &_recv_over._over, 0);
		//cout << retval << endl;
	}

	void do_send(void* packet)
	{
		//cout << "Send" << endl;

		OVER_EXP* sdata = new OVER_EXP{ reinterpret_cast<char*>(packet) };
		int retval = WSASend(_socket, &sdata->_wsabuf, 1, 0, 0, &sdata->_over, 0);
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
		_vl.lock();
		if (_view_list.count(c_id) == 0) {
			_vl.unlock();
			return;
		}
		_view_list.erase(c_id);
		_vl.unlock();

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
	_vl.lock();
	if (_view_list.count(c_id) != 0) {
		_vl.unlock();
		SC_MOVE_PLAYER_PACKET p;
		p.id = c_id;
		p.size = sizeof(SC_MOVE_PLAYER_PACKET);
		p.type = SC_MOVE_PLAYER;
		p.x = clients[c_id].x;
		p.y = clients[c_id].y;
		p.move_time = clients[c_id]._last_move_time;
		do_send(&p);
	}
	else {
		_vl.unlock();
		send_add_player_packet(c_id);
	}
}

void SESSION::send_add_player_packet(int c_id)
{
	_vl.lock();
	if (_view_list.count(c_id) != 0) {
		_vl.unlock();
		send_move_packet(c_id);
		return;
	}
	_view_list.insert(c_id);
	_vl.unlock();

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

		clients[c_id]._vl.lock();
		auto old_vl = clients[c_id]._view_list;
		clients[c_id]._vl.unlock();

		unordered_set <int> new_vl;
		for (auto& cl : clients) {
			if (cl._state != ST_INGAME) continue;
			if (cl._id == c_id) continue;
			if (can_see(cl._id, c_id))
				new_vl.insert(cl._id);
		}
		for (auto& o : new_vl) {
			//전에 없었는데 새로 생기면 add player packer보냄
			if (old_vl.count(o) == 0) {
				clients[o].send_add_player_packet(c_id);
				clients[c_id].send_add_player_packet(o);
			}
			//전에도 있고 지금도 있으면 move packet
			else {
				clients[o].send_move_packet(c_id);
				clients[c_id].send_move_packet(o);

			}
		}


		/*for (auto& cl : clients) {
			if (cl._state != ST_INGAME) continue;
			cl.send_move_packet(c_id);

		}*/
		clients[c_id].send_move_packet(c_id);

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
				cout << "GQCS Error on client[" << key << "]\n";
				disconnect(static_cast<int>(key));
				if (ex_over->_comp_type == OP_SEND) delete ex_over;
				continue;
			}
		}

		if ((0 == num_bytes) && ((ex_over->_comp_type == OP_RECV) || (ex_over->_comp_type == OP_SEND))) {
			disconnect(static_cast<int>(key));
			if (ex_over->_comp_type == OP_SEND) delete ex_over;
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
//constexpr int VIEW_RANGE = 4;
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
//	concurrency::concurrent_unordered_set<int> _view_list;
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
//		int retval = WSARecv(_socket, &_recv_over._wsabuf, 1, 0, &recv_flag,&_recv_over._over, 0);
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
//		//_vl.lock();
//		if (_view_list.count(c_id) == 0) {
//			//_vl.unlock();
//			return;
//		}
//
//		unordered_set <int> old_vl;
//		_vl.lock();
//		for (auto& cl : _view_list) {
//			if (cl == c_id) {
//				continue;
//			}
//			old_vl.insert(cl);
//		}
//		_view_list.clear();
//		for (auto& old_cl : old_vl) {
//			_view_list.insert(old_cl);
//		}
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
//	//_vl.lock();
//	if (_view_list.count(c_id) != 0) {
//		//_vl.unlock();
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
//		//_vl.unlock();
//		send_add_player_packet(c_id);
//	}
//}
//
//void SESSION::send_add_player_packet(int c_id)
//{
//	//_vl.lock();
//	if (_view_list.count(c_id) != 0) {
//		//_vl.unlock();
//		send_move_packet(c_id);
//		return;
//	}
//	_view_list.insert(c_id);
//	//_vl.unlock();
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
//			if (can_see(cl._id, c_id))
//				new_vl.insert(cl._id);
//		}
//		for (auto& o : new_vl) {
//			//전에 없었는데 새로 생기면 add player packer보냄
//			if (old_vl.count(o) == 0) {
//				clients[o].send_add_player_packet(c_id);
//				clients[c_id].send_add_player_packet(o);
//			}
//			//전에도 있고 지금도 있으면 move packet
//			else {
//				clients[o].send_move_packet(c_id);
//				clients[c_id].send_move_packet(o);
//
//			}
//		}
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
