#include <array>
#include <random>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
#include <unordered_set>
#include <queue>
#include <string>
#include <cmath>
#include "Player.h"
#include "AStar.h"
#include "SQL.h"

extern "C"
{
#include "include/lua.h"
#include "include/lauxlib.h"
#include "include/lualib.h"
}
#pragma comment(lib, "lua54.lib")


using namespace std;
using namespace chrono;

SQL sql;

unordered_set<int>** sector_list;
mutex** sector_mutex;

HANDLE h_iocp;
enum EVENT_TYPE { EV_SLEEP, EV_MOVE, EV_ATTACK, EV_FOLLOW, EV_DIRECTION };
class EVENT {
public:
	int object_id;
	EVENT_TYPE type;
	chrono::system_clock::time_point exec_time;
	EVENT() {}
	EVENT(int id, EVENT_TYPE t, chrono::system_clock::time_point tp) : object_id(id), type(t), exec_time(tp) {}
	~EVENT() {}
	bool operator < (const EVENT& e) const
	{
		return exec_time > e.exec_time;
	}
};
priority_queue<EVENT> timer_queue;
mutex timer_mtx;

enum COMPLETION_TYPE { OP_ACCEPT, OP_RECV, OP_SEND, OP_NPC };
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
void do_npc(int, EVENT_TYPE);
atomic <int> player_count = 0;

enum SESSION_STATE { ST_FREE, ST_ALLOC, ST_INGAME };
class SESSION {
	OVER_EXP recv_over;
	
public:
	SOCKET socket;
	int id;
	unsigned int prev_move_time;
	unsigned int prev_attack_time;
	int	remain_data;
	Player player;
	
	SESSION_STATE state;
	mutex state_mtx;
	unordered_set<int> view_list;
	mutex view_list_mtx;
	lua_State* lua;
	mutex	lua_mtx;
	
	atomic_bool	is_active_npc;
	int enemy_id;

	SESSION() {
		state = ST_FREE;
		socket = 0;
		id = -1;
		prev_move_time = 0;
		remain_data = 0;
	}
	~SESSION() {}

	void do_recv() {
		//cout << walker_id << " do_recv\n";
		DWORD recv_flag = 0;
		memset(&recv_over.over, 0, sizeof(recv_over.over));
		recv_over.wsa_buf.len = BUFSIZE - remain_data;
		recv_over.wsa_buf.buf = recv_over.data + remain_data;
		int ret = WSARecv(socket, &recv_over.wsa_buf, 1, 0, &recv_flag,&recv_over.over, 0);
		if (0 != ret) {
			int err_no = WSAGetLastError();
			if (WSA_IO_PENDING != err_no) {
				//cout << "WSARecv Error : " << err_no << " Client: " << walker_id << endl;
				disconnect(id);
			}
		}
	}
		
	void do_send(void* packet) {
		OVER_EXP* send_over = new OVER_EXP(reinterpret_cast<char*>(packet));
		int ret = WSASend(socket, &send_over->wsa_buf, 1, 0, 0, &send_over->over, 0);
		if (0 != ret) {
			int err_no = WSAGetLastError();
			if (WSA_IO_PENDING != err_no) {
				//cout << "WSASend Error : " << err_no << " Client: " << walker_id << endl;
				delete send_over;
				disconnect(id);
			}
		}
	}
	void send_login_ok_packet();
	void send_login_fail_packet();
	void send_login_info_packet();
	void send_move_packet(int);
	void send_direction_packet(int);
	void send_attack_packet(int, bool, bool);
	void send_in_packet(int);
	void send_out_packet(int);
	void send_chat_packet(int c_id, const char* mess);
	void send_stat_packet();

	void insert_view_list(int);
	void erase_view_list(int);
};
array<SESSION, MAX_USER + MAX_NPC> objects;

bool is_npc(int id) 
{
	if (id >= MAX_USER) return true;
	return false;
}

void SESSION::send_login_ok_packet()
{
	SC_LOGIN_OK_PACKET packet;
	do_send(&packet);
}

void SESSION::send_login_fail_packet()
{
	SC_LOGIN_FAIL_PACKET packet;
	do_send(&packet);
}

void SESSION::send_login_info_packet()
{
	if (is_npc(this->id))
		return;
	
	player_count.fetch_add(1);
	//cout << "Player Count : " << player_count << endl;
	
	SC_LOGIN_INFO_PACKET packet;
	packet.id = id;
	packet.x = player.position.x;
	packet.y = player.position.y;
	packet.hp = player.hp;
	packet.max_hp = player.max_hp;
	packet.level = player.level;
	packet.exp = player.exp;
	do_send(&packet);
}

void SESSION::send_move_packet(int walker_id)
{
	if (is_npc(this->id))
		return;
	
	SC_MOVE_OBJECT_PACKET packet;
	packet.id = walker_id;
	packet.x = objects[walker_id].player.position.x;
	packet.y = objects[walker_id].player.position.y;
	packet.time = objects[walker_id].prev_move_time;
	do_send(&packet);
}

void SESSION::send_direction_packet(int watcher_id)
{
	if (is_npc(this->id))
		return;
	
	SC_DIRECTION_PACKET packet;
	packet.id = watcher_id;
	packet.direction = objects[watcher_id].player.direction;
	do_send(&packet);
}

void SESSION::send_attack_packet(int attacker_id, bool hit, bool dead)
{
	if (is_npc(this->id))
		return;
	
	SC_ATTACK_PACKET packet;
	packet.id = attacker_id;
	packet.hit = hit;
	packet.dead = dead;
	do_send(&packet);
}

void SESSION::send_in_packet(int entered_client_id)
{
	if (is_npc(this->id))
		return;

	SC_ADD_OBJECT_PACKET packet;
	packet.id = entered_client_id;
	packet.x = objects[entered_client_id].player.position.x;
	packet.y = objects[entered_client_id].player.position.y;
	memcpy(&packet.name, &objects[entered_client_id].player.name, sizeof(packet.name));
	do_send(&packet);
}

void SESSION::send_out_packet(int left_one_id)
{
	if (is_npc(this->id))
		return;
	
	SC_REMOVE_OBJECT_PACKET packet;
	packet.id = left_one_id;
	do_send(&packet);
}

void SESSION::send_chat_packet(int talker_id, const char* message)
{
	if (is_npc(this->id))
		return;

	SC_CHAT_PACKET packet;
	packet.id = talker_id;
	strcpy_s(packet.mess, message);
	do_send(&packet);
}

void SESSION::send_stat_packet()
{
	if (is_npc(this->id))
		return;

	SC_STAT_CHANGE_PACKET packet;
	packet.hp = player.hp;
	packet.max_hp = player.max_hp;
	packet.level = player.level;
	packet.exp = player.exp;
	do_send(&packet);
}

void SESSION::insert_view_list(int id)
{
	view_list_mtx.lock();
	view_list.insert(id);
	view_list_mtx.unlock();
}

void SESSION::erase_view_list(int id)
{
	view_list_mtx.lock();
	view_list.erase(id);
	view_list_mtx.unlock();
}

void initialize_sector_list()
{
	sector_list = new unordered_set<int>*[SECTOR_NUM];
	for (int i = 0; i < SECTOR_NUM; ++i) {
		sector_list[i] = new unordered_set<int>[SECTOR_NUM];
	}
	for (int i = 0; i < SECTOR_NUM; ++i) {
		for (int j = 0; j < SECTOR_NUM; ++j) {
		}
	}
	
	sector_mutex = new mutex * [SECTOR_NUM];
	for (int i = 0; i < SECTOR_NUM; ++i) {
		sector_mutex[i] = new mutex[SECTOR_NUM];
	}
}

void remove_from_sector_list(int id)
{
	int x = objects[id].player.position.x / SECTOR_SIZE;
	int y = objects[id].player.position.y / SECTOR_SIZE;
	sector_mutex[x][y].lock();
	sector_list[x][y].erase(id);
	sector_mutex[x][y].unlock();
}

void add_to_sector_list(int id)
{
	int x = objects[id].player.position.x / SECTOR_SIZE;
	int y = objects[id].player.position.y / SECTOR_SIZE;
	sector_mutex[x][y].lock();
	sector_list[x][y].insert(id);
	sector_mutex[x][y].unlock();
}

void get_from_sector_list(int id, unordered_set<int>& sector)
{
	int x = objects[id].player.position.x / SECTOR_SIZE;
	int y = objects[id].player.position.y / SECTOR_SIZE;
	
	// get list from around 9 sectors
	for (int i = -1; i <= 1; ++i) {
		if (x + i < 0 || x + i >= SECTOR_NUM)
			continue;
		for (int j = -1; j <= 1; ++j) {
			if (y + j < 0 || y + j >= SECTOR_NUM)
				continue;
			
			sector_mutex[x + i][y + j].lock();
			sector.insert(sector_list[x + i][y + j].begin(), sector_list[x + i][y + j].end());
			sector_mutex[x + i][y + j].unlock();
		}
	}
}

void show_all_sector_list()
{
	system("cls");
	int total = 0;
	for (int i = 0; i < SECTOR_NUM; ++i) {
		for (int j = 0; j < SECTOR_NUM; ++j) {
			printf("%2d ", sector_list[i][j].size());
			total += sector_list[i][j].size();
		}
		cout << endl;
	}
	cout << "total: " << total << endl;
}

void disconnect(int id)
{
	SESSION* this_player = &objects[id];
	
	if (!is_npc(id)) {	//npc아니면 저장
		sql.save_info(this_player->player.name, this_player->player.level, this_player->player.exp, this_player->player.hp, this_player->player.max_hp, this_player->player.position.x, this_player->player.position.y);
	}
	
	{
		lock_guard<mutex> m{ objects[id].state_mtx };
		if (this_player->state == ST_FREE) return;
		else this_player->state = ST_FREE;
	}
	
	this_player->view_list_mtx.lock();
	unordered_set<int> view_list = this_player->view_list;
	this_player->view_list_mtx.unlock();

	for (auto& client_in_view : view_list) {
		objects[client_in_view].send_out_packet(id);
		objects[client_in_view].erase_view_list(id);
	}

	remove_from_sector_list(id);

	if (is_npc(id)) {
		this_player->is_active_npc = false;
		return;
	}
	closesocket(this_player->socket);

	player_count.fetch_sub(1);
	if (player_count.load() < 50) {
		cout << player_count.load() << " Clients Remain. Client id: " << id << endl;
	}
}

TI randomly_spawn_player()
{
	random_device rd;
	default_random_engine dre(rd());
	uniform_int_distribution <int>spawn_location(0, W_WIDTH);
	return TI { spawn_location(dre), spawn_location(dre) };
}

bool in_eyesight(int p1, int p2)
{
	if (abs(objects[p1].player.position.x - objects[p2].player.position.x) >= VIEW_RANGE) return false;
	if (abs(objects[p1].player.position.y - objects[p2].player.position.y) >= VIEW_RANGE) return false;
	return true;
}

bool attack_position(int attacker, int defender)
{
	if (objects[attacker].player.position.x == objects[defender].player.position.x && objects[attacker].player.position.y == objects[defender].player.position.y) { 
		return true;
	}
	if (objects[attacker].player.position.x + objects[attacker].player.tc_direction.x == objects[defender].player.position.x && objects[attacker].player.position.y + objects[attacker].player.tc_direction.y == objects[defender].player.position.y) {
		return true;
	}
	return false;
}

int get_new_client_id()
{
	for (int i = 0; i < objects.size(); i++) {
		lock_guard<mutex> m(objects[i].state_mtx);
		if (objects[i].state == ST_FREE) return i;
	}
	return -1;
}

void reserve_timer(int id, EVENT_TYPE event_type, int time)
{
	EVENT event(id, event_type, chrono::system_clock::now() + chrono::milliseconds(time));
	timer_mtx.lock();
	timer_queue.push(event);
	timer_mtx.unlock();
}

void wake_up_npc(int npc_id)
{
	if (objects[npc_id].is_active_npc) return;
	bool old_state = false;
	if (false == atomic_compare_exchange_strong(&objects[npc_id].is_active_npc, &old_state, true))
		return;
	//cout << npc_id << " 일어나라 NPC야 " << objects[npc_id].is_active_npc << endl;
	reserve_timer(npc_id, EV_MOVE, 500);
}

void npc_talk(int npc_id, int id)
{
	/*if (objects[npc_id].player.position.x != objects[id].player.position.x || objects[npc_id].player.position.y != objects[id].player.position.y)
		return;*/
	/*objects[npc_id].lua_mtx.lock();
	auto L = objects[npc_id].lua;
	lua_getglobal(L, "event_player_move");
	lua_pushnumber(L, id);
	lua_pcall(L, 1, 0, 0);
	lua_pop(L, 1);
	objects[npc_id].lua_mtx.unlock();*/
}

void process_packet(int id, char* packet)
{
	switch (packet[2]) {
	case CS_MOVE:
	{
		SESSION* moved_client = &objects[id];
		CS_MOVE_PACKET* recv_packet = reinterpret_cast<CS_MOVE_PACKET*>(packet);
		
		if (moved_client->prev_move_time + 250 > recv_packet->move_time) return;		//1초 지나야 움직일 수 있음

		remove_from_sector_list(id);
		
		moved_client->player.key_input = recv_packet->direction;
		moved_client->player.key_check();
		moved_client->prev_move_time = recv_packet->move_time;
		moved_client->send_move_packet(id);								//본인 움직임 보냄
		
		add_to_sector_list(id);
		unordered_set<int> list_of_sector;
		get_from_sector_list(id, list_of_sector);						//본인이 보는 섹터에 있는 클라이언트들을 불러옴

		unordered_set<int> new_view_list;										//새로 업데이트 할 뷰 리스트
		moved_client->view_list_mtx.lock();
		unordered_set<int> old_view_list = moved_client->view_list;				//이전 뷰 리스트 복사
		moved_client->view_list_mtx.unlock();

		for (auto& client_in_sector : list_of_sector) {
			{
				lock_guard<mutex> m(objects[id].state_mtx);
				if (objects[client_in_sector].state != ST_INGAME) continue;
			}
			if (objects[client_in_sector].id == id) continue;
			if (in_eyesight(id, client_in_sector)) {						//현재 시야 안에 있는 클라이언트
				new_view_list.insert(client_in_sector);								//new list 채우기
			}
		}
		
		for (auto& new_one : new_view_list) {
			if (old_view_list.count(new_one) == 0) {							//새로 시야에 들어온 플레이어
				objects[new_one].insert_view_list(id);			//시야에 들어온 플레이어의 뷰 리스트에 움직인 플레이어 추가
				moved_client->send_in_packet(new_one);
				objects[new_one].send_in_packet(id);

				objects[new_one].send_direction_packet(id);
				moved_client->send_direction_packet(new_one);
				
				if (is_npc(new_one)) {									//NPC라면 깨우기
					wake_up_npc(new_one);
				}
			}
			else
				objects[new_one].send_move_packet(id);						//기존에 있었으면 무브

			if (is_npc(new_one)) {
				npc_talk(new_one, id);
			}
		}
		for (auto& old_one : old_view_list) {
			if (new_view_list.count(old_one) == 0) {								//시야에서 사라진 플레이어
				objects[old_one].erase_view_list(id);					//시야에서 사라진 플레이어의 뷰 리스트에서 움직인 플레이어 삭제
				
				moved_client->send_out_packet(old_one);							//움직인 플레이어한테 목록에서 사라진 플레이어 삭제 패킷 보냄
				objects[old_one].send_out_packet(id);							//시야에서 사라진 플레이어에게 움직인 플레이어 삭제 패킷 보냄
			}
		}
		moved_client->view_list_mtx.lock();
		moved_client->view_list = new_view_list;										//뷰 리스트 갱신
		moved_client->view_list_mtx.unlock();
	}
	break;
	case CS_DIRECTION:
	{
		SESSION* watcher = &objects[id];
		CS_DIRECTION_PACKET* recv_packet = reinterpret_cast<CS_DIRECTION_PACKET*>(packet);

		watcher->player.direction = recv_packet->direction;
		switch (watcher->player.direction) {
		case DIR_UP: watcher->player.tc_direction = { 0, -1 }; break;
		case DIR_DOWN: watcher->player.tc_direction = { 0, 1 }; break;
		case DIR_LEFT: watcher->player.tc_direction = { -1, 0 }; break;
		case DIR_RIGHT: watcher->player.tc_direction = { 1, 0 }; break;
		}
		
		watcher->view_list_mtx.lock();
		unordered_set<int> watcher_view_list = watcher->view_list;
		watcher->view_list_mtx.unlock();

		for (auto& watched_id : watcher_view_list) {
			objects[watched_id].send_direction_packet(id);
		}
	}
	break;
	case CS_ATTACK:
	{
		SESSION* attacker = &objects[id];
		CS_ATTACK_PACKET* recv_packet = reinterpret_cast<CS_ATTACK_PACKET*>(packet);
		
		if (attacker->prev_attack_time + 500 > recv_packet->time) return;
		attacker->prev_attack_time = recv_packet->time;
		//cout << attacker->id << " attack" << endl;

		unordered_set<int> attacker_view_list;
		unordered_set<int> list_of_sector;
		get_from_sector_list(id, list_of_sector);
		for (auto& client_in_sector : list_of_sector) {
			{
				lock_guard<mutex> m(objects[id].state_mtx);
				if (objects[client_in_sector].state != ST_INGAME) continue;
			}
			if (client_in_sector == id) continue;
			if (in_eyesight(id, client_in_sector)) {						//현재 시야 안에 있는 클라이언트
				attacker_view_list.insert(client_in_sector);								//new list 채우기
			}
		}
		
		//맞았는지 안맞았는지 판단하고 그에 맞는 패킷 보내기
		bool hit = false;
		bool dead = false;
		for (auto& watcher : attacker_view_list) {
			if (attack_position(id, watcher)) {
				hit = true;
				objects[watcher].player.decrease_hp(50);

				if (is_npc(watcher)) {
					objects[watcher].enemy_id = id;
				}

				if (objects[watcher].player.hp <= 0) {							//죽었을 때
					dead = true;
					int defender_level = objects[watcher].player.level;
					attacker->player.increase_exp(defender_level * 50);
					attacker->send_stat_packet();

					disconnect(watcher);
				}
				objects[watcher].send_stat_packet();							//맞은놈 스탯
			}
		}

		for (auto& watcher : attacker_view_list) {
			objects[watcher].send_attack_packet(id, hit, dead);		//공격 모션
		}
		attacker->send_attack_packet(id, hit, dead);					//맞는 소리
	}
	break;
	case CS_LOGIN:
	{
		SESSION* new_client = &objects[id];
		CS_LOGIN_PACKET* recv_packet = reinterpret_cast<CS_LOGIN_PACKET*>(packet);

		PI info = sql.find_by_name(recv_packet->name);
		if (info.level == -1) {
			cout << "새로 계정 생성\n";
			// 새로운 계정 생성
			TI pos = randomly_spawn_player();
			sql.insert_new_account(recv_packet->name, 1, 0, 100, 100, pos.x, pos.y);
			info = sql.find_by_name(recv_packet->name);
		}
		else {
			cout << "기존 계정 사용\n";
		}
		// 기존 계정 로드
		memcpy(new_client->player.name, info.name, sizeof(new_client->player.name));
		new_client->player.direction = DIR_DOWN;
		new_client->player.level = info.level;
		new_client->player.exp = info.exp;
		new_client->player.hp = info.hp;
		new_client->player.max_hp = info.max_hp;
		new_client->player.position.x = info.x;
		new_client->player.position.y = info.y;
		//cout << info.name << " " << info.level << " " << info.exp << " " << info.hp << " " << info.x << " " << info.y << endl;

		new_client->send_login_info_packet();										//새로온 애한테 로그인 됐다고 전송
		{
			lock_guard<mutex> m{ objects[id].state_mtx };
			objects[id].state = ST_INGAME;
		}

		for (auto& old_client : objects) {
			{
				lock_guard<mutex> m(objects[id].state_mtx);
				if (old_client.state != ST_INGAME) continue;
			}
			if (id == old_client.id) continue;

			if (in_eyesight(id, old_client.id)) {
				add_to_sector_list(id);
				
				old_client.insert_view_list(id);				//시야 안에 들어온 클라의 View list에 새로온 놈 추가
				new_client->insert_view_list(old_client.id);	//새로온 놈의 viewlist에 시야 안에 들어온 클라 추가
				
				new_client->send_in_packet(old_client.id);				//새로 들어온 클라에게 시야 안의 기존 애들 위치 전송
				old_client.send_in_packet(id);							//시야 안에 클라한테 새로온 애 위치 전송.
				
				new_client->send_direction_packet(old_client.id);			//새로 들어온 클라에게 시야 안의 기존 애들 방향 전송
				old_client.send_direction_packet(id);						//시야 안에 클라한테 새로온 애 방향 전송.
				
				if (is_npc(old_client.id)) {							//NPC라면 깨우기
					wake_up_npc(old_client.id);
				}
			}
		}
	}
	break;
	case CS_CHAT:
	{
		SESSION* mumbling_client = &objects[id];
		CS_CHAT_PACKET* recv_packet = reinterpret_cast<CS_CHAT_PACKET*>(packet);
		
		mumbling_client->view_list_mtx.lock();
		unordered_set<int> mumbling_view_list = mumbling_client->view_list;
		mumbling_client->view_list_mtx.unlock();
		
		for (auto& hearing_client : mumbling_view_list) {
			objects[hearing_client].send_chat_packet(id, recv_packet->mess);
		}
		cout << "client " << id << " : " << recv_packet->mess << endl;
	}
	break;
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

		switch (ex_over->completion_type) {
		case OP_SEND: 
		{
			delete ex_over;
			break;
		}
		case OP_RECV:
		{
			int data_to_proccess = num_bytes + objects[key].remain_data;
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
			objects[key].remain_data = data_to_proccess;
			if (data_to_proccess > 0) {
				memcpy(ex_over->data, packet, data_to_proccess);
			}
			objects[key].do_recv();
			break;
		}
		case OP_ACCEPT: 
		{
			int id = get_new_client_id();
			if (id != -1) {
				{
					lock_guard<mutex> m(objects[id].state_mtx);
					objects[id].state = ST_ALLOC;
				}
				objects[id].id = id;
				objects[id].socket = global_client_socket;
				objects[id].remain_data = 0;
				objects[id].view_list_mtx.lock();
				objects[id].view_list.clear();
				objects[id].view_list_mtx.unlock();

				CreateIoCompletionPort(reinterpret_cast<HANDLE>(global_client_socket), h_iocp, id, 0);
				objects[id].do_recv();
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
		case OP_NPC: {
			EVENT_TYPE event_type = ex_over->event_type;
			do_npc(static_cast<int>(key), event_type);
			delete ex_over;
			break;
		}
		default: cout << "Unknown Completion Type" << endl; break;
		}
	}
}

int API_get_x(lua_State* L)
{
	int user_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int x = objects[user_id].player.position.x;
	lua_pushnumber(L, x);
	return 1;
}

int API_get_y(lua_State* L)
{
	int user_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int y = objects[user_id].player.position.y;
	lua_pushnumber(L, y);
	return 1;
}

int API_SendMessage(lua_State* L)
{
	int my_id = (int)lua_tointeger(L, -3);
	int user_id = (int)lua_tointeger(L, -2);
	char* mess = (char*)lua_tostring(L, -1);

	lua_pop(L, 4);
	//cout << "send message" << endl;
	objects[user_id].send_chat_packet(my_id, mess);
	return 0;
}

void spawn_npc()
{
	random_device rd;
	default_random_engine dre(rd());
	uniform_int_distribution <int>random_personality(0, 2);
	
	auto start_t = chrono::system_clock::now();
	for (int npc_id = MAX_USER; npc_id < MAX_USER + MAX_NPC; ++npc_id) {
		objects[npc_id].id = npc_id;
		objects[npc_id].view_list.clear();
		objects[npc_id].state = ST_INGAME;
		objects[npc_id].player.position = randomly_spawn_player();
		objects[npc_id].player.personality = static_cast<PERSONALITY>(random_personality(dre));
		objects[npc_id].enemy_id = -1;
		sprintf_s(objects[npc_id].player.name, "NPC %d", npc_id);
		add_to_sector_list(npc_id);
		
		//auto L = objects[npc_id].lua = luaL_newstate();
		//luaL_openlibs(L);
		//luaL_loadfile(L, "npc.lua");
		//int error = lua_pcall(L, 0, 0, 0);
		//if (error) {
		//	cout << "Error:" << lua_tostring(L, -1);
		//	lua_pop(L, 1);
		//}

		//lua_getglobal(L, "set_uid");
		//lua_pushnumber(L, npc_id);
		//lua_pcall(L, 1, 0, 0);
		//// lua_pop(L, 1);// eliminate set_uid from stack after call

		//lua_register(L, "API_SendMessage", API_SendMessage);
		//lua_register(L, "API_get_x", API_get_x);
		//lua_register(L, "API_get_y", API_get_y);
	}
	auto end_t = chrono::system_clock::now();
	auto duration = chrono::duration_cast<chrono::milliseconds>(end_t - start_t);
	cout << "NPC spawn time : " << duration.count() << " ms" << endl;
}

void do_npc(int npc_id, EVENT_TYPE event_type)
{
	SESSION* this_npc = &objects[npc_id];

	if (!this_npc->is_active_npc)
		return;

	random_device rd;
	default_random_engine dre(rd());
	
	
	switch (event_type) {
	case EV_MOVE:
	{
		//cout << "NPC " << npc_id << " move" << endl;
		unordered_set<int> new_view_list;									//새로 업데이트 할 뷰 리스트
		this_npc->view_list_mtx.lock();
		unordered_set<int> old_view_list = this_npc->view_list;			//이전 뷰 리스트 복사
		this_npc->view_list_mtx.unlock();

		if (old_view_list.size() == 0) {
			//cout << npc_id << " 잔다 NPC\n";
			this_npc->is_active_npc = false;
			return;
		}

		remove_from_sector_list(npc_id);
		
		uniform_int_distribution <int>random_direction(1, 8);
		this_npc->player.key_input = random_direction(dre);

		this_npc->player.key_check();
		this_npc->prev_move_time = static_cast<unsigned>(duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count());

		add_to_sector_list(npc_id);
		unordered_set<int> list_of_sector;
		get_from_sector_list(npc_id, list_of_sector);				//본인이 보는 섹터에 있는 클라이언트들을 불러옴

		for (auto& client_in_sector : list_of_sector) {
			if (is_npc(client_in_sector)) continue;
			{
				lock_guard<mutex> m(objects[npc_id].state_mtx);
				if (objects[client_in_sector].state != ST_INGAME) continue;
			}
			if (objects[client_in_sector].id == npc_id) continue;
			if (in_eyesight(npc_id, client_in_sector)) {							//현재 시야 안에 있는 클라이언트
				new_view_list.insert(client_in_sector);								//new list 채우기
			}
		}

		for (auto& new_one : new_view_list) {
			if (old_view_list.count(new_one) == 0) {					//새로 시야에 들어온 플레이어
				objects[new_one].insert_view_list(npc_id);		//시야에 들어온 플레이어의 뷰 리스트에 움직인 플레이어 추가
				objects[new_one].send_in_packet(npc_id);					//새로 시야에 들어온 플레이어에게 움직인 플레이어 정보 전송
			}
			else {
				objects[new_one].send_move_packet(npc_id);
			}
		}
		for (auto& old_one : old_view_list) {
			if (new_view_list.count(old_one) == 0) {					//시야에서 사라진 플레이어
				objects[old_one].send_out_packet(npc_id);			//시야에서 사라진 플레이어에게 움직인 플레이어 삭제 패킷 보냄
				objects[old_one].erase_view_list(npc_id);		//시야에서 사라진 플레이어의 뷰 리스트에서 움직인 플레이어 삭제
			}
		}  
		this_npc->view_list_mtx.lock();
		this_npc->view_list = new_view_list;								//뷰 리스트 갱신
		this_npc->view_list_mtx.unlock();

		if (new_view_list.size() == 0) {
			//cout << npc_id << " 잔다 NPC\n";
			this_npc->is_active_npc = false;
			return;
		}
		//cout << npc_id << " moving\n";
		
		if (new_view_list.find(this_npc->enemy_id) != new_view_list.end()) {
			//cout << npc_id << " 적 발견\n";
			reserve_timer(npc_id, EV_FOLLOW, NPC_MOVE_TIME);
			return;
		}
		uniform_int_distribution <int>random_action(0, 1);
		if (random_action(dre) == 0) {
			reserve_timer(npc_id, EV_MOVE, NPC_MOVE_TIME);
			return;
		}
		else {
			reserve_timer(npc_id, EV_SLEEP, NPC_MOVE_TIME);
			return;
		}
		return;
	}
	break;
	case EV_ATTACK:
	{
		if (!attack_position(npc_id, this_npc->enemy_id)) {		//거리는 되는데 방향이 안맞을 경우
			reserve_timer(npc_id, EV_DIRECTION, NPC_MOVE_TIME);
			return;
		}
		//cout << npc_id << " attack\n";
		unordered_set<int> attacker_view_list;
		unordered_set<int> list_of_sector;
		get_from_sector_list(npc_id, list_of_sector);
		for (auto& client_in_sector : list_of_sector) {
			{
				lock_guard<mutex> m(objects[npc_id].state_mtx);
				if (objects[client_in_sector].state != ST_INGAME) continue;
			}
			if (client_in_sector == npc_id) continue;
			if (in_eyesight(npc_id, client_in_sector)) {						//현재 시야 안에 있는 클라이언트
				attacker_view_list.insert(client_in_sector);								//new list 채우기
			}
		}

		//맞았는지 안맞았는지 판단하고 그에 맞는 패킷 보내기
		bool hit = false;
		bool dead = false;
		for (auto& watcher : attacker_view_list) {
			if (attack_position(npc_id, watcher)) {
				hit = true;
				objects[watcher].player.decrease_hp(50);
				
				//objects[watcher].enemy_id = npc_id;		//npc 끼리도 적 가능

				if (objects[watcher].player.hp <= 0) {							//죽었을 때
					if(this_npc->enemy_id == watcher)
						this_npc->enemy_id = -1;
					
					dead = true;
					int defender_level = objects[watcher].player.level;
					this_npc->player.increase_exp(defender_level * 50);
					disconnect(watcher);
				}
				objects[watcher].send_stat_packet();							//맞은놈 스탯
			}
		}

		for (auto& watcher : attacker_view_list) {
			objects[watcher].send_attack_packet(npc_id, hit, dead);		//공격 모션
		}
		
		if (attacker_view_list.find(this_npc->enemy_id) != attacker_view_list.end()) {		//적이 있으면 계속 따라가기
			//cout << npc_id << " 적 발견\n";
			reserve_timer(npc_id, EV_FOLLOW, NPC_MOVE_TIME);
			return;
		}
		reserve_timer(npc_id, EV_MOVE, NPC_MOVE_TIME);
		return;
	}
	break;
	case EV_FOLLOW:
	{
		//cout << npc_id << " follow\n";
		unordered_set<int> new_view_list;									//새로 업데이트 할 뷰 리스트
		this_npc->view_list_mtx.lock();
		unordered_set<int> old_view_list = this_npc->view_list;			//이전 뷰 리스트 복사
		this_npc->view_list_mtx.unlock();

		if (old_view_list.size() == 0) {
			//cout << npc_id << " 잔다 NPC\n";
			this_npc->is_active_npc = false;
			return;
		}
		
		//this_npc->enemy_id = *old_view_list.begin();
		if (this_npc->enemy_id == -1) {					//적이 없다면
			//this_npc->enemy_id = *old_view_list.begin();
			reserve_timer(npc_id, EV_MOVE, NPC_MOVE_TIME);
			return;
		}
		TI my_position{ this_npc->player.position.x, this_npc->player.position.y };
		TI enemy_position{ objects[this_npc->enemy_id].player.position.x, objects[this_npc->enemy_id].player.position.y };
		
		if (my_position.x > enemy_position.x) {
			my_position.x -= enemy_position.x;
			enemy_position.x = 0;
		}
		else if((my_position.x < enemy_position.x)) {
			enemy_position.x -= my_position.x;
			my_position.x = 0;
		}
		else {
			my_position.x = 0;
			enemy_position.x = 0;
		}
		
		if (my_position.y > enemy_position.y) {
			my_position.y -= enemy_position.y;
			enemy_position.y = 0;
		}
		else if ((my_position.y < enemy_position.y)) {
			enemy_position.y -= my_position.y;
			my_position.y = 0;
		}
		else {
			my_position.y = 0;
			enemy_position.y = 0;
		}
		
		TI diff = { abs(enemy_position.x - my_position.x) + 1, abs(enemy_position.y - my_position.y) + 1 };
		if(diff.x + diff.y <= 3){		//가로나 세로 딱 붙어있는 위치
			//cout << npc_id << " 공격 범위\n";
			reserve_timer(npc_id, EV_ATTACK, NPC_MOVE_TIME);
			return;
		}
		TI next_position = turn_astar(my_position, enemy_position, diff);

		remove_from_sector_list(npc_id);
		
		if (my_position.y > next_position.y) {
			this_npc->player.tc_direction.y = -1;
		}
		if (my_position.y < next_position.y) {
			this_npc->player.tc_direction.y = 1;
		}
		if (my_position.x > next_position.x) {
			this_npc->player.tc_direction.x = -1;
		}
		if (my_position.x < next_position.x) {
			this_npc->player.tc_direction.x = 1;
		}
		
		this_npc->player.dir_check();
		this_npc->prev_move_time = static_cast<unsigned>(duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count());

		add_to_sector_list(npc_id);
		unordered_set<int> list_of_sector;
		get_from_sector_list(npc_id, list_of_sector);				//본인이 보는 섹터에 있는 클라이언트들을 불러옴

		for (auto& client_in_sector : list_of_sector) {
			if (is_npc(client_in_sector)) continue;
			{
				lock_guard<mutex> m(objects[npc_id].state_mtx);
				if (objects[client_in_sector].state != ST_INGAME) continue;
			}
			if (objects[client_in_sector].id == npc_id) continue;
			if (in_eyesight(npc_id, client_in_sector)) {							//현재 시야 안에 있는 클라이언트
				new_view_list.insert(client_in_sector);								//new list 채우기
			}
		}

		for (auto& new_one : new_view_list) {
			if (old_view_list.count(new_one) == 0) {					//새로 시야에 들어온 플레이어
				objects[new_one].insert_view_list(npc_id);		//시야에 들어온 플레이어의 뷰 리스트에 움직인 플레이어 추가
				objects[new_one].send_in_packet(npc_id);					//새로 시야에 들어온 플레이어에게 움직인 플레이어 정보 전송
			}
			else {
				objects[new_one].send_move_packet(npc_id);
			}
		}
		for (auto& old_one : old_view_list) {
			if (new_view_list.count(old_one) == 0) {					//시야에서 사라진 플레이어
				objects[old_one].send_out_packet(npc_id);			//시야에서 사라진 플레이어에게 움직인 플레이어 삭제 패킷 보냄
				objects[old_one].erase_view_list(npc_id);		//시야에서 사라진 플레이어의 뷰 리스트에서 움직인 플레이어 삭제
			}
		}
		this_npc->view_list_mtx.lock();
		this_npc->view_list = new_view_list;								//뷰 리스트 갱신
		this_npc->view_list_mtx.unlock();

		if (new_view_list.size() == 0) {
			//cout << npc_id << " 잔다 NPC\n";
			this_npc->is_active_npc = false;
			return;
		}

		if (new_view_list.find(this_npc->enemy_id) != new_view_list.end()) {		//적이 있으면 계속 따라가기
			//cout << npc_id << " 적 발견\n";
			reserve_timer(npc_id, EV_FOLLOW, NPC_MOVE_TIME);
			return;
		}
		reserve_timer(npc_id, EV_MOVE, NPC_MOVE_TIME);
		return;
	}
	case EV_SLEEP:
	{
		//cout << npc_id << " 쉰다 NPC\n";
		uniform_int_distribution <int>random_action(0, 1);
		if (random_action(dre) == 0) {
			reserve_timer(npc_id, EV_MOVE, NPC_MOVE_TIME);
			return;
		}
		else {
			reserve_timer(npc_id, EV_SLEEP, NPC_MOVE_TIME);
			return;
		}
	}
	case EV_DIRECTION:
	{
		if (this_npc->player.position.x > objects[this_npc->enemy_id].player.position.x) {
			this_npc->player.tc_direction.x = -1;
			this_npc->player.direction = DIR_LEFT;
		}
		else if (this_npc->player.position.x < objects[this_npc->enemy_id].player.position.x) {
			this_npc->player.tc_direction.x = 1;
			this_npc->player.direction = DIR_RIGHT;
		}
		else {
			this_npc->player.tc_direction.x = 0;
		}
		if (this_npc->player.position.y > objects[this_npc->enemy_id].player.position.y) {
			this_npc->player.tc_direction.y = -1;
			this_npc->player.direction = DIR_UP;
		}
		else if (this_npc->player.position.y < objects[this_npc->enemy_id].player.position.y) {
			this_npc->player.tc_direction.y = 1;
			this_npc->player.direction = DIR_DOWN;
		}
		else {
			this_npc->player.tc_direction.y = 0;
		}
		
		this_npc->view_list_mtx.lock();
		unordered_set<int> npc_view_list = this_npc->view_list;
		this_npc->view_list_mtx.unlock();

		for (auto& watched_id : npc_view_list) {
			objects[watched_id].send_direction_packet(npc_id);
		}
		
		//std::cout << this_npc->player.position.x << ", " << this_npc->player.position.y << " " << (int)this_npc->player.tc_direction.x << ", " << (int)this_npc->player.tc_direction.y << " enemy : " << objects[this_npc->enemy_id].player.position.x << ", " << objects[this_npc->enemy_id].player.position.y << "\n";
		reserve_timer(npc_id, EV_FOLLOW, NPC_MOVE_TIME);
		return;
	}
	break;
	}

	cout << npc_id << " 이벤트 끝\n";
	cout << event_type << endl;
}

void do_timer()
{
	EVENT event;
	int num_excuted_npc{};
	auto start_t = chrono::high_resolution_clock::now();
	
	while (1) {
		timer_mtx.lock();
		if (timer_queue.empty()) {										//아무것도 없으면 슬립
			timer_mtx.unlock();
			this_thread::sleep_for(1ms);
			continue;
		}
		else {
			event = timer_queue.top();									//맨 위에 있는거 꺼내서
		}
		timer_mtx.unlock();
		if (event.exec_time > chrono::system_clock::now()) {			//지금 시간보다 크면 
			this_thread::sleep_for(1ms);						//더 기다리기
			continue;
		}
		
		timer_mtx.lock();
		timer_queue.pop();
		timer_mtx.unlock();
		
		OVER_EXP* over = new OVER_EXP();
		over->event_type = event.type;
		over->completion_type = OP_NPC;
		PostQueuedCompletionStatus(h_iocp, 1, event.object_id, &over->over);
		
		//cout << event.object_id << " position : " << objects[event.object_id].player.position.x << ", " << objects[event.object_id].player.position.y << " event : " << event.type << endl;
		/*++num_excuted_npc;
		if (chrono::high_resolution_clock::now() - start_t > chrono::milliseconds(NPC_MOVE_TIME)) {
			cout << num_excuted_npc << "개의 NPC가 움직임\n";
			num_excuted_npc = 0;
			start_t = chrono::high_resolution_clock::now();
		}*/
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
	server_addr.sin_port = htons(PORT_NUM);
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
	
	initialize_sector_list();
	spawn_npc();
	thread timer{ do_timer };
	
	vector <thread> worker_threads;
	int num_threads = thread::hardware_concurrency();
	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(work_thread);
	for (auto& th : worker_threads)
		th.join();

	timer.join();
	
	closesocket(global_server_socket);
	WSACleanup();
}
