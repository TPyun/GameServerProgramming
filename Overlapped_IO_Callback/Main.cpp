#include <array>
#include <random>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_set>
#include <concurrent_unordered_set.h>
#include "Player.h"

using namespace std;
//Overlapped IO Callback
void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED send_over, DWORD recv_flag);
void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED recv_over, DWORD recv_flag);

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

TI randomly_spawn_player();
void disconnect(int);
void process_packet(int client_id, char* packet);

enum SESSION_STATE { FREE, ALLOC, INGAME };
class SESSION {
public:
	OVER_EXP recv_over;
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

	~SESSION() {
		closesocket(socket);
	}

	bool do_recv() {
		//cout << client_id << " do_recv\n";
		DWORD recv_flag = 0;
		ZeroMemory(&recv_over.over, sizeof(recv_over.over));
		recv_over.wsa_buf.len = BUFSIZE - remain_data;
		recv_over.wsa_buf.buf = recv_over.data + remain_data;
		recv_over.over.hEvent = reinterpret_cast<HANDLE>(client_id);
		cout << "do recv\n";
		int ret = WSARecv(socket, &recv_over.wsa_buf, 1, 0, &recv_flag, &recv_over.over, recv_callback);
		if (0 != ret) {
			int err_no = WSAGetLastError();
			cout << "pending\n";
			if (WSA_IO_PENDING != err_no) {
				cout << "WSARecv Error : " << err_no << " Client: " << client_id << endl;
				disconnect(client_id);
				return true;
			}
		}
		return false;
	}
		
	bool do_send(void* packet) {
		OVER_EXP* send_over = new OVER_EXP(reinterpret_cast<char*>(packet));
		send_over->over.hEvent = reinterpret_cast<HANDLE>(client_id);
		int ret = WSASend(socket, &send_over->wsa_buf, 1, 0, 0, &send_over->over, send_callback);
		if (0 != ret) {
			int err_no = WSAGetLastError();
			if (WSA_IO_PENDING != err_no) {
				cout << "WSASend Error : " << err_no << " Client: " << client_id << endl;
				delete send_over;
				disconnect(client_id);
				return true;
			}
		}
		return false;
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
	player.position = randomly_spawn_player();
	SC_LOGIN_PACKET packet;
	packet.client_id = client_id;
	packet.position = player.position;
	if (do_send(&packet)) return;
}

void SESSION::send_move_packet(int client_id)
{
	SC_MOVE_PACKET packet;
	packet.client_id = client_id;
	packet.position = clients[client_id].player.position;
	packet.time = clients[client_id].prev_time;
	if (do_send(&packet)) return;
}

void SESSION::send_out_packet(int client_id)
{
	SC_OUT_PACKET packet;
	packet.client_id = client_id;
	if (do_send(&packet)) return;
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
		{
			lock_guard<mutex> m{ clients[client_id].state_mtx };
			if (clients[client_in_view].state == FREE) continue;
		}
		clients[client_in_view].send_out_packet(client_id);
	}
	closesocket(clients[client_id].socket);
}

TI randomly_spawn_player()
{
	random_device rd;
	default_random_engine dre(rd());
	uniform_int_distribution <int>spawn_location(0, MAP_SIZE);
	return TI{ spawn_location(dre), spawn_location(dre) };
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

void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED send_over, DWORD recv_flag)
{
	OVER_EXP* exp_over = reinterpret_cast<OVER_EXP*>(send_over);	//PACKET 클래스의 첫번째 변수 주소를 가지고 PACKET 클래스 포인터를 찾음
	int sent_client_id = reinterpret_cast<int>(exp_over->over.hEvent);
	delete exp_over;

	if (err != 0) {
		cout << "Send Callback Error " << err << " Client_ID: " << sent_client_id << endl;
		disconnect(sent_client_id);
		return;
	}
}

void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED recv_over, DWORD recv_flag)
{
	//cout << "recv callback\n";
	OVER_EXP* exp_over = reinterpret_cast<OVER_EXP*>(recv_over);
	int client_id = reinterpret_cast<int>(exp_over->over.hEvent);
	
	if (err != 0) {
		cout << "Recv Callback Error: " << err << " Client_ID: " << client_id << endl;
		disconnect(client_id);
		return;
	}

	int data_to_proccess = num_bytes + clients[client_id].remain_data;
	char* packet = exp_over->data;
	while (data_to_proccess > 0) {
		int packet_size = packet[0];
		if (packet_size <= data_to_proccess) {
			process_packet(static_cast<int>(client_id), packet);
			packet += packet_size;
			data_to_proccess -= packet_size;
		}
		else break;
	}
	clients[client_id].remain_data = data_to_proccess;
	if (data_to_proccess > 0) {
		memcpy(exp_over->data, packet, data_to_proccess);
	}
	
	//cout << client_id << endl;
	bool should_out = false;
	{
		lock_guard<mutex> m(clients[client_id].state_mtx);
		if (clients[client_id].state != FREE) should_out = true;
	}
	if (!should_out)
		clients[client_id].do_recv();
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

		int num_clients = 0;
		for (auto& client : clients) {
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

			if (old_view_list.count(new_one) == 0) {			//새로 시야에 들어온 플레이어
				clients[new_one].insert_view_list(client_id);//시야에 들어온 플레이어의 뷰 리스트에 움직인 플레이어 추가
			}
		}

		moved_client->view_list_mtx.lock();
		moved_client->view_list = new_view_list;					//새로운 뷰 리스트로 갈아치움
		moved_client->view_list_mtx.unlock();
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

	for (;;) {
		SOCKET client_socket = WSAAccept(server_socket, reinterpret_cast<sockaddr*>(&server_addr), &addr_size, 0, 0);
		int client_id = get_new_client_id();
		if (client_id != -1) {
			{
				lock_guard<mutex> m(clients[client_id].state_mtx);
				clients[client_id].state = ALLOC;
			}
			clients[client_id].client_id = client_id;
			clients[client_id].socket = client_socket;
			clients[client_id].remain_data = 0;

			clients[client_id].view_list_mtx.lock();
			clients[client_id].view_list.clear();
			clients[client_id].view_list_mtx.unlock();

			clients[client_id].do_recv();
		}
		else {
			cout << "Max user exceeded.\n";
		}
	}

	closesocket(server_socket);
	WSACleanup();
}
