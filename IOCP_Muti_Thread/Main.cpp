#include <array>
#include <random>
#include <vector>
#include <thread>
#include <mutex>
#include<chrono>
#include <atomic>
#include <unordered_set>
#include <queue>
#include "Player.h"
using namespace std;

HANDLE h_iocp;

enum EVENT_TYPE { MOVE, ATTACK };
class EVENT {
public:
	int object_id;
	EVENT_TYPE type;
	chrono::system_clock::time_point exec_time;

	EVENT(int id, EVENT_TYPE t, chrono::system_clock::time_point tp) : object_id(id), type(t), exec_time(tp) {}
	~EVENT() {}
	bool operator < (const EVENT& e) const
	{
		return exec_time > e.exec_time;
	}
};
priority_queue<EVENT> timer_queue;
mutex timer_mtx;

enum COMPLETION_TYPE { OP_ACCEPT, OP_RECV, OP_SEND, OP_AI };
class OVER_EXP {
public:
	WSAOVERLAPPED over;	//클래스의 맨 첫번째에 있어야만 함. 이거를 가지고 클래스 포인터 위치 찾을거임
	WSABUF wsa_buf;
	char data[BUFSIZE];
	COMPLETION_TYPE completion_type;
	EVENT_TYPE event_type;
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
void move_npc(int);
void call_npc(int);
atomic <int> player_count = 0;

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
			}
		}
	}
	
	void send_login_packet();
	void send_move_packet(int);
	void send_out_packet(int);
	void insert_view_list(int);
	void erase_view_list(int);
};
array<SESSION, MAX_USER + MAX_NPC> clients;

void SESSION::send_login_packet()
{
	player_count.fetch_add(1);
	//cout << "Player Count : " << player_count << endl;
	
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
		if (clients[client_id].state == FREE) return;
	}
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

	player_count.fetch_sub(1);
	//cout << "Player Count : " << player_count << endl;
}

TI randomly_spawn_player()
{
	random_device rd;
	default_random_engine dre(rd());
	uniform_int_distribution <int>spawn_location(0, MAP_SIZE);
	return TI { spawn_location(dre), spawn_location(dre) };
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

		moved_client->send_move_packet(client_id);					//본인 움직임 보냄

		unordered_set<int> new_view_list;	//새로 업데이트 할 뷰 리스트
		moved_client->view_list_mtx.lock();
		unordered_set<int> old_view_list = moved_client->view_list;	//이전 뷰 리스트 복사
		moved_client->view_list_mtx.unlock();

		for (int user_id = 0; user_id < MAX_USER; user_id++) {
			{
				lock_guard<mutex> m(clients[client_id].state_mtx);
				if (clients[user_id].state != INGAME) continue;
			}
			if (clients[user_id].client_id == client_id) continue;

			if (in_eyesight(client_id, clients[user_id].client_id)) {	//현재 시야 안에 있는 클라이언트
				new_view_list.insert(clients[user_id].client_id);			//new list 채우기
			}
		}
		for (auto& new_one : new_view_list) {
			clients[new_one].send_move_packet(client_id);
			if (old_view_list.count(new_one) == 0) {						//새로 시야에 들어온 플레이어
				clients[new_one].insert_view_list(client_id);		//시야에 들어온 플레이어의 뷰 리스트에 움직인 플레이어 추가
				moved_client->send_move_packet(new_one);
				moved_client->insert_view_list(new_one);		//뷰 리스트에 추가
			}
		}
		for (auto& old_one : old_view_list) {
			if (new_view_list.count(old_one) == 0) {			//시야에서 사라진 플레이어
				moved_client->send_out_packet(old_one);		//움직인 플레이어한테 목록에서 사라진 플레이어 삭제 패킷 보냄
				clients[old_one].send_out_packet(client_id);		//시야에서 사라진 플레이어에게 움직인 플레이어 삭제 패킷 보냄
				moved_client->erase_view_list(old_one);		//뷰 리스트에서 삭제
				clients[old_one].erase_view_list(client_id);	//시야에서 사라진 플레이어의 뷰 리스트에서 움직인 플레이어 삭제
			}
		}

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

		for (auto& old_client : clients) {
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

void work_thread()
{
	while (true) {
		DWORD num_bytes{};
		ULONG_PTR key;
		WSAOVERLAPPED* over = nullptr;
		//완료된 상태를 가져옴
		BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);
		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);
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
			delete ex_over;
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
		case OP_AI: {
			EVENT_TYPE event = ex_over->event_type;

			move_npc(static_cast<int>(key));
			delete ex_over;
			break;
		}
		default: cout << "Unknown Completion Type" << endl; break;
		}
	}
}

void spawn_npc()
{
	auto start_t = chrono::system_clock::now();
	for (int npc_id = MAX_USER; npc_id < MAX_USER + MAX_NPC; ++npc_id) {
		clients[npc_id].client_id = npc_id;
		clients[npc_id].view_list.clear();
		clients[npc_id].state = INGAME;
		clients[npc_id].player.position = randomly_spawn_player();

		EVENT event(npc_id, MOVE, chrono::system_clock::now() + chrono::milliseconds(npc_id - MAX_USER + 1));
		timer_mtx.lock();
		timer_queue.push(event);
		timer_mtx.unlock();

	}
	auto end_t = chrono::system_clock::now();
	auto duration = chrono::duration_cast<chrono::milliseconds>(end_t - start_t);
	cout << "NPC spawn time : " << duration.count() << " ms" << endl;
}

void move_npc(int npc_id)
{
	//cout << "move npc "<< npc_id << endl;
	bool skip_ai = true;
	for (int user_id = 0; user_id < MAX_USER; ++user_id) {	//움직이기 전에 시야처리 해서 시야에 있는 유저가 있으면 움직임
		{
			lock_guard<mutex> m(clients[user_id].state_mtx);
			if (clients[user_id].state != INGAME) continue;
		}
		if (true == in_eyesight(npc_id, user_id)) {
			skip_ai = false;
			break;
		}
	}
	if (true == skip_ai) {
		/*EVENT event(npc_id, MOVE, chrono::system_clock::now() + chrono::milliseconds(10));
		timer_mtx.lock();
		timer_queue.push(event);
		timer_mtx.unlock();*/
		return;
	}
	//cout << "move_npc" << npc_id << endl;
	SESSION* moved_npc = &clients[npc_id];
	int direction = rand() % 4;
	switch (direction)
	{
	case 0: moved_npc->player.key_input.up = true; break;
	case 1: moved_npc->player.key_input.down = true; break;
	case 2: moved_npc->player.key_input.left = true; break;
	case 3: moved_npc->player.key_input.right = true; break;
	}
	moved_npc->player.key_check();
	
	unordered_set<int> new_view_list;	//새로 업데이트 할 뷰 리스트
	moved_npc->view_list_mtx.lock();
	unordered_set<int> old_view_list = moved_npc->view_list;	//이전 뷰 리스트 복사
	moved_npc->view_list_mtx.unlock();

	for (int user_id = 0; user_id < MAX_USER; ++user_id) {		//유저만 검색
		{
			lock_guard<mutex> m(clients[user_id].state_mtx);
			if (clients[user_id].state != INGAME) continue;
		}
		//if (clients[i].client_id == npc_id) continue;
		if (in_eyesight(npc_id, clients[user_id].client_id)) {	//현재 시야 안에 있는 클라이언트
			new_view_list.insert(clients[user_id].client_id);			//new list 채우기
			}
	}
	for (auto& new_one : new_view_list) {
		clients[new_one].send_move_packet(npc_id);
		cout << "move packet" << endl;
		if (old_view_list.count(new_one) == 0) {						//새로 시야에 들어온 플레이어
			clients[new_one].insert_view_list(npc_id);		//시야에 들어온 플레이어의 뷰 리스트에 움직인 플레이어 추가
			moved_npc->insert_view_list(new_one);		//뷰 리스트에 추가
		}
	}
	for (auto& old_one : old_view_list) {
		if (new_view_list.count(old_one) == 0) {			//시야에서 사라진 플레이어
			clients[old_one].send_out_packet(npc_id);		//시야에서 사라진 플레이어에게 움직인 플레이어 삭제 패킷 보냄
			moved_npc->erase_view_list(old_one);		//뷰 리스트에서 삭제
			clients[old_one].erase_view_list(npc_id);	//시야에서 사라진 플레이어의 뷰 리스트에서 움직인 플레이어 삭제
		}
	}

	EVENT event(npc_id, MOVE, chrono::system_clock::now() + chrono::milliseconds(10));
	timer_mtx.lock();
	timer_queue.push(event);
	timer_mtx.unlock();
}

void do_timer()
{
	while (1) {
		timer_mtx.lock();
		if (timer_queue.size() == 0) {
			timer_mtx.unlock();
			this_thread::sleep_for(10ms);
			continue;
		}
		timer_mtx.unlock();

		auto event = timer_queue.top();					//맨 위에 있는거 꺼내서
		if (event.exec_time > chrono::system_clock::now()) {	//지금 시간보다 크면 
			this_thread::sleep_for(10ms);				//더 기다리고 아니면 실행
			continue;
		}
		
		timer_mtx.lock();
		timer_queue.pop();
		timer_mtx.unlock();

		OVER_EXP* over = new OVER_EXP();
		over->event_type = event.type;
		over->completion_type = OP_AI;
		PostQueuedCompletionStatus(h_iocp, 1, event.object_id, &over->over);
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
	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	//먼저 IOCP 생성한걸 ExistingCompletionPort에 넣어줌. key는 임의로 아무거나. 마지막 것은 무시
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(global_server_socket), h_iocp, 9999, 0);
	global_client_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	global_accept_over.completion_type = OP_ACCEPT;
	AcceptEx(global_server_socket, global_client_socket, global_accept_over.data, 0, addr_size + 16, addr_size + 16, 0, &global_accept_over.over);
	
	spawn_npc();
	thread timer{ do_timer };
	//thread ai_thread{ do_ai, h_iocp };
	
	vector <thread> worker_threads;
	int num_threads = thread::hardware_concurrency();
	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(work_thread);
	for (auto& th : worker_threads)
		th.join();

	//ai_thread.join();
	timer.join();
	
	closesocket(global_server_socket);
	WSACleanup();
}

