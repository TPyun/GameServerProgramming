#include <array>
#include <random>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
//#include <unordered_set>
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
mutex sql_mtx;

unordered_set<int>** sector_list;
mutex** sector_mutex;

HANDLE h_iocp;
enum EVENT_TYPE { EV_SLEEP, EV_MOVE, EV_ATTACK, EV_FOLLOW, EV_DIRECTION, EV_NATURAL_HEALING_FOR_PLAYERS, EV_RESPAWN };
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

enum COMPLETION_TYPE { OP_ACCEPT, OP_RECV, OP_SEND, OP_NPC, OP_NATURAL_HEALING, OP_RESPAWN };
class OVER_EXP {
public:
	WSAOVERLAPPED over;	//Ŭ������ �� ù��°�� �־�߸� ��. �̰Ÿ� ������ Ŭ���� ������ ��ġ ã������
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
TI random_spawn_location();
void disconnect(int);
void do_npc(int, EVENT_TYPE);
//atomic <int> player_count = 0;

enum SESSION_STATE { ST_FREE, ST_ALLOC, ST_INGAME, ST_DEAD };
class SESSION {
	OVER_EXP recv_over;
	
public:
	SOCKET socket;
	int id;
	unsigned int prev_move_time;
	unsigned int prev_attack_time;
	
	char process_field[BUFSIZE];
	int	remain_data_size;
	Player player;
	
	SESSION_STATE state;
	mutex state_mtx;
	unordered_set<int> view_list;
	mutex view_list_mtx;
	lua_State* lua;
	mutex lua_mtx;

	atomic_bool	is_natural_healing;
	atomic_bool	is_active_npc;
	int enemy_id;

	SESSION() {
		state = ST_FREE;
		socket = 0;
		id = -1;
		prev_move_time = 0;
		remain_data_size = 0;
	}
	~SESSION() {}

	void do_recv() {
		//cout << walker_id << " do_recv\n";
		DWORD recv_flag = 0;
		memset(&recv_over.over, 0, sizeof(recv_over.over));
		recv_over.wsa_buf.len = BUFSIZE - remain_data_size;
		//recv_over.wsa_buf.buf = recv_over.data;
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
	void send_attack_packet(int, bool, bool, ATTACK_TYPE);
	void send_in_packet(int);
	void send_out_packet(int);
	void send_chat_packet(int c_id, const char* mess);
	void send_stat_packet();
	
	void dead();
	void respawn();

	void insert_view_list(int);
	void erase_view_list(int);

	void get_from_sector_list(unordered_set<int>& sector);
	void move_sector();
	void add_to_sector_list();
	void remove_from_sector_list(TI);
};
array<SESSION, MAX_USER + MAX_NPC + MAX_OBSTACLE> objects;

char get_object_type(int id) 
{
	if (id < MAX_USER)
		return 0;
	else if (id < MAX_USER + MAX_NPC)
		return 1;
	else if (id < MAX_USER + MAX_NPC + MAX_OBSTACLE)
		return 2;
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
	if (get_object_type(this->id) != 0)
		return;
	
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
	if (get_object_type(this->id) != 0)
		return;
	
	SC_MOVE_OBJECT_PACKET packet;
	packet.id = walker_id;
	packet.x = objects[walker_id].player.position.x;
	packet.y = objects[walker_id].player.position.y;
	packet.time = objects[walker_id].prev_move_time;
	do_send(&packet);
}

void SESSION::send_direction_packet(int walker_id)
{
	if (get_object_type(this->id) != 0)
		return;
	
	SC_DIRECTION_PACKET packet;
	packet.id = walker_id;
	packet.direction = objects[walker_id].player.direction;
	do_send(&packet);
}

void SESSION::send_attack_packet(int attacker_id, bool hit, bool dead, ATTACK_TYPE attack_type)
{
	if (get_object_type(this->id) != 0)
		return;
	
	SC_ATTACK_PACKET packet;
	packet.id = attacker_id;
	if (dead)
		packet.hit_type = HIT_TYPE_DEAD;
	else if (hit)
		packet.hit_type = HIT_TYPE_HIT;
	else
		packet.hit_type = HIT_TYPE_NONE;

	packet.attack_type = attack_type;
	do_send(&packet);
}

void SESSION::send_in_packet(int entered_client_id)
{
	if (get_object_type(this->id) != 0)
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
	if (get_object_type(this->id) != 0)
		return;
	
	SC_REMOVE_OBJECT_PACKET packet;
	packet.id = left_one_id;
	do_send(&packet);
}

void SESSION::send_chat_packet(int talker_id, const char* message)
{
	if (get_object_type(this->id) != 0)
		return;

	SC_CHAT_PACKET packet;
	packet.id = talker_id;
	strcpy_s(packet.mess, message);
	packet.size -= CHAT_SIZE;
	packet.size += strlen(message) + 1;
	do_send(&packet);
}

void SESSION::send_stat_packet()
{
	if (get_object_type(this->id) != 0)
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

void SESSION::remove_from_sector_list(TI position)
{
	int id = this->id;
	int x = position.x / SECTOR_SIZE;
	int y = position.y / SECTOR_SIZE;
	sector_mutex[x][y].lock();
	sector_list[x][y].erase(id);
	sector_mutex[x][y].unlock();
}

void  SESSION::add_to_sector_list()
{
	int id = this->id;
	int x = objects[id].player.position.x / SECTOR_SIZE;
	int y = objects[id].player.position.y / SECTOR_SIZE;
	sector_mutex[x][y].lock();
	sector_list[x][y].insert(id);
	sector_mutex[x][y].unlock();
}

void SESSION::move_sector()
{
	int id = this->id;
	SESSION* this_player = &objects[id];
	TI prev_pos = this_player->player.position;
	int prev_x = prev_pos.x / SECTOR_SIZE;
	int prev_y = prev_pos.y / SECTOR_SIZE;
	
	this_player->player.key_check();
	TI curr_pos = this_player->player.position;
	int curr_x = curr_pos.x / SECTOR_SIZE;
	int curr_y = curr_pos.y / SECTOR_SIZE;
	
	if (prev_x != curr_x || prev_y != curr_y) {
		add_to_sector_list();
		remove_from_sector_list(prev_pos);
	}
}

void SESSION::get_from_sector_list(unordered_set<int>& sector)
{
	int id = this->id;
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

bool collide_check(TI location)
{
	if (location.x < 0 || location.x >= W_WIDTH || location.y < 0 || location.y >= W_HEIGHT)
		return true;
	
	int x = location.x / SECTOR_SIZE;
	int y = location.y / SECTOR_SIZE;
	
	unordered_set<int> sector;
	sector_mutex[x][y].lock();
	sector.insert(sector_list[x][y].begin(), sector_list[x][y].end());
	sector_mutex[x][y].unlock();

	for (auto& id : sector) {
		if (get_object_type(id) != 2) continue;
		if (objects[id].player.position.x == location.x && objects[id].player.position.y == location.y) return true;
	}
	return false;
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
	{
		lock_guard<mutex> m{ this_player->state_mtx };
		if (this_player->state == ST_FREE) return;
		else this_player->state = ST_FREE;
	}

	if (get_object_type(id) == 0 && this_player->player.level != -1) {	//�÷��̾�� ����
		sql_mtx.lock();
		sql.save_info(this_player->player.name, this_player->player.level, this_player->player.exp, this_player->player.hp, this_player->player.max_hp, this_player->player.position.x, this_player->player.position.y);
		sql_mtx.unlock();
	}
	
	this_player->view_list_mtx.lock();
	unordered_set<int> view_list = this_player->view_list;
	this_player->view_list_mtx.unlock();

	for (auto& client_in_view : view_list) {
		objects[client_in_view].send_out_packet(id);
		objects[client_in_view].erase_view_list(id);
	}

	this_player->remove_from_sector_list(this_player->player.position);
	this_player->is_natural_healing = false;
	this_player->is_active_npc = false;

	ZeroMemory(this_player->player.name, NAME_SIZE);

	if (get_object_type(id) != 0)
		return;

	closesocket(this_player->socket);

	/*player_count.fetch_sub(1);
	if (player_count.load() < 50) {
		cout << player_count.load() << " Clients Remain. Client id: " << id << endl;
	}*/
}

TI random_spawn_location()
{
	random_device rd;
	default_random_engine dre(rd());
	uniform_int_distribution <int>spawn_location(0, W_WIDTH);
	
	bool collide = true;
	TI location;

	while (collide) {
		location.x = spawn_location(dre);
		location.y = spawn_location(dre);
		collide = collide_check(location);
	}
	return location;
}

bool in_eyesight(int p1, int p2)
{
	if (abs(objects[p1].player.position.x - objects[p2].player.position.x) > VIEW_RANGE) return false;
	if (abs(objects[p1].player.position.y - objects[p2].player.position.y) > VIEW_RANGE) return false;
	return true;
}

bool in_aggr_range(int p1, int p2)
{
	if (abs(objects[p1].player.position.x - objects[p2].player.position.x) > AGGR_RANGE) return false;
	if (abs(objects[p1].player.position.y - objects[p2].player.position.y) > AGGR_RANGE) return false;
	return true;
}

bool attack_position(int attacker, int defender, ATTACK_TYPE attack_type)
{
	switch (attack_type)
	{
	case ATTACK_FORWARD:
		if (objects[attacker].player.tc_direction.x != 0 && objects[attacker].player.tc_direction.y != 0)
			objects[attacker].player.tc_direction.y = 0;
		if (objects[attacker].player.position.x == objects[defender].player.position.x && objects[attacker].player.position.y == objects[defender].player.position.y)
			return true;
		if (objects[attacker].player.position.x + objects[attacker].player.tc_direction.x == objects[defender].player.position.x && objects[attacker].player.position.y + objects[attacker].player.tc_direction.y == objects[defender].player.position.y)
			return true;
		break;

	case ATTACK_WIDE:
		for (int x = -1; x <= 1; x++) {
			for (int y = -1; y <= 1; y++) {
				if (x * y != 0) continue;
				if (objects[attacker].player.position.x + x == objects[defender].player.position.x && objects[attacker].player.position.y + y == objects[defender].player.position.y)
					return true;
			}
		}
		break;
	default: cout << "Attack Position Error\n"; 
		break;
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
	//cout << npc_id << " �Ͼ�� NPC�� " << objects[npc_id].is_active_npc << endl;
	reserve_timer(npc_id, EV_MOVE, 500);
}

void natural_healing_start(int id)
{
	if (objects[id].is_natural_healing) return;
	if(objects[id].player.hp == objects[id].player.max_hp) return;
	bool old_state = false;
	if (false == atomic_compare_exchange_strong(&objects[id].is_natural_healing, &old_state, true))
		return;
	reserve_timer(id, EV_NATURAL_HEALING_FOR_PLAYERS, NATURAL_HEALING_TIME);
}

void SESSION::dead()
{
	int id = this->id;
	SESSION* this_player = &objects[id];

	{
		lock_guard<mutex> m{ this_player->state_mtx };
		this_player->state = ST_DEAD;
	}
	
	if (get_object_type(id) == 0) {	//�÷��̾�� ����
		sql_mtx.lock();
		sql.save_info(this_player->player.name, this_player->player.level, this_player->player.exp, this_player->player.hp, this_player->player.max_hp, this_player->player.position.x, this_player->player.position.y);
		sql_mtx.unlock();
	}

	this_player->view_list_mtx.lock();
	unordered_set<int> view_list = this_player->view_list;
	this_player->view_list_mtx.unlock();

	for (auto& client_in_view : view_list) {
		objects[client_in_view].send_out_packet(id);
		objects[client_in_view].erase_view_list(id);
	}

	this_player->remove_from_sector_list(this_player->player.position);
	this_player->is_natural_healing = false;
	this_player->is_active_npc = false;

	reserve_timer(id, EV_RESPAWN, RESPAWN_TIME);
}

void SESSION::respawn()
{
	int id = this->id;
	SESSION* this_player = &objects[id];

	{
		lock_guard<mutex> m{ this_player->state_mtx };
		this_player->state = ST_INGAME;
	}
	
	this_player->view_list_mtx.lock();
	unordered_set<int> view_list = this_player->view_list;
	this_player->view_list_mtx.unlock();
	
	for (auto& client_in_view : view_list) {
		this_player->send_out_packet(client_in_view);
	}
	
	this_player->view_list_mtx.lock();
	this_player->view_list.clear();
	this_player->view_list_mtx.unlock();
	
	this_player->player.hp = this_player->player.max_hp;
	this_player->player.position = random_spawn_location();
	add_to_sector_list();

	this_player->send_login_info_packet();

	unordered_set<int> list_of_sector;
	this_player->get_from_sector_list(list_of_sector);

	for (auto& old_client_id : list_of_sector) {
		SESSION* old_client = &objects[old_client_id];
		if (id == old_client->id) continue;

		if (in_eyesight(id, old_client->id)) {

			old_client->insert_view_list(id);				//�þ� �ȿ� ���� Ŭ���� View list�� ���ο� �� �߰�
			this_player->insert_view_list(old_client->id);	//���ο� ���� viewlist�� �þ� �ȿ� ���� Ŭ�� �߰�

			this_player->send_in_packet(old_client->id);				//���� ���� Ŭ�󿡰� �þ� ���� ���� �ֵ� ��ġ ����
			old_client->send_in_packet(id);							//�þ� �ȿ� Ŭ������ ���ο� �� ��ġ ����.

			this_player->send_direction_packet(old_client->id);			//���� ���� Ŭ�󿡰� �þ� ���� ���� �ֵ� ���� ����
			old_client->send_direction_packet(id);						//�þ� �ȿ� Ŭ������ ���ο� �� ���� ����.

			if (get_object_type(old_client->id) == 1) {							//NPC��� �����
				wake_up_npc(old_client->id);
			}
		}
	}
}

void error(lua_State* L, const char* fmt, ...) 
{
	va_list argp;
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
	lua_close(L);
	exit(EXIT_FAILURE);
}

void npc_talk(int npc_id, int id)
{
	objects[npc_id].lua_mtx.lock();
	
	if (objects[npc_id].player.position.x != objects[id].player.position.x || objects[npc_id].player.position.y != objects[id].player.position.y) {
		objects[npc_id].lua_mtx.unlock();
		return;
	}
	lua_State* L = objects[npc_id].lua;
	lua_getglobal(L, "npc_talk");
	lua_pushnumber(L, id);	//�Լ� ���� �ֱ�
	lua_pcall(L, 1, 0, 0);	//�Լ� ȣ��
	//lua_pop(L, 1);
	objects[npc_id].lua_mtx.unlock();
}

TI key_to_dir(char key)
{
	TI dir{};
	switch (key)
	{
	case KEY_UP_LEFT:
		dir = { -1, -1 };
		break;
	case KEY_UP:
		dir = { 0, -1 };
		break;
	case KEY_UP_RIGHT:
		dir = { 1, -1 };
		break;
	case KEY_LEFT:
		dir = { -1, 0 };
		break;
	case KEY_NONE:
		dir = { 0, 0 };
		break;
	case KEY_RIGHT:
		dir = { 1, 0 };
		break;
	case KEY_DOWN_LEFT:
		dir = { -1, 1 };
		break;
	case KEY_DOWN:
		dir = { 0, 1 };
		break;
	case KEY_DOWN_RIGHT:
		dir = { 1, 1 };
		break;
	}
	return dir;
}

void process_packet(int id, char* packet)
{
	{
		lock_guard<mutex> m{ objects[id].state_mtx};
		if (objects[id].state == ST_DEAD) return;
	}
	switch (packet[2]) {
	case CS_MOVE:
	{
		SESSION* moved_client = &objects[id];
		CS_MOVE_PACKET* recv_packet = reinterpret_cast<CS_MOVE_PACKET*>(packet);
		
		if (moved_client->prev_move_time + PLAYER_MOVE_TIME > recv_packet->move_time) return;		//1�� ������ ������ �� ����

		//�浹�˻�
		TI direction = key_to_dir(recv_packet->direction);
		if (collide_check({ direction.x + moved_client->player.position.x, direction.y + moved_client->player.position.y })) {
			//�浹�ص� �̵��ϴ� ���������� ������ ��ȯ��
			moved_client->prev_move_time = recv_packet->move_time;
			moved_client->send_move_packet(id);	//���� ������� ����

			if (moved_client->player.tc_direction.x != direction.x || moved_client->player.tc_direction.y != direction.y) {
				moved_client->view_list_mtx.lock();
				unordered_set<int> view_list = moved_client->view_list;
				moved_client->view_list_mtx.unlock();

				moved_client->player.key_input = recv_packet->direction;
				moved_client->player.key_check(false);
				moved_client->send_direction_packet(id);
				for (auto& watcher : view_list) {
					objects[watcher].send_direction_packet(id);
				}
				return;
			}
			return;
		}
		
		TI prev_pos = moved_client->player.position;
		moved_client->player.key_input = recv_packet->direction;
		moved_client->move_sector();
		
		moved_client->prev_move_time = recv_packet->move_time;
		moved_client->send_move_packet(id);								//���� ������ ����
		
		unordered_set<int> list_of_sector;
		moved_client->get_from_sector_list(list_of_sector);						//������ ���� ���Ϳ� �ִ� Ŭ���̾�Ʈ���� �ҷ���

		unordered_set<int> new_view_list;										//���� ������Ʈ �� �� ����Ʈ
		moved_client->view_list_mtx.lock();
		unordered_set<int> old_view_list = moved_client->view_list;				//���� �� ����Ʈ ����
		moved_client->view_list_mtx.unlock();

		for (auto& client_in_sector : list_of_sector) {
			//{
			//	//lock_guard<mutex> m(objects[client_in_sector].state_mtx);
			//	if (objects[client_in_sector].state != ST_INGAME) continue;
			//}
			if (objects[client_in_sector].id == id) continue;
			if (in_eyesight(id, client_in_sector)) {						//���� �þ� �ȿ� �ִ� Ŭ���̾�Ʈ
				new_view_list.insert(client_in_sector);								//new list ä���
			}
		}
		
		for (auto& new_one : new_view_list) {
			if (old_view_list.count(new_one) == 0) {							//���� �þ߿� ���� �÷��̾�
				objects[new_one].insert_view_list(id);			//�þ߿� ���� �÷��̾��� �� ����Ʈ�� ������ �÷��̾� �߰�
				moved_client->send_in_packet(new_one);
				objects[new_one].send_in_packet(id);

				objects[new_one].send_direction_packet(id);
				moved_client->send_direction_packet(new_one);
				
				if (get_object_type(new_one) == 1) {									//NPC��� �����
					wake_up_npc(new_one);
				}
			}
			else
				objects[new_one].send_move_packet(id);						//������ �־����� ����

			if (get_object_type(new_one) == 1) {
				npc_talk(new_one, id);
			}
		}
		for (auto& old_one : old_view_list) {
			if (new_view_list.count(old_one) == 0) {								//�þ߿��� ����� �÷��̾�
				objects[old_one].erase_view_list(id);					//�þ߿��� ����� �÷��̾��� �� ����Ʈ���� ������ �÷��̾� ����
				
				moved_client->send_out_packet(old_one);							//������ �÷��̾����� ��Ͽ��� ����� �÷��̾� ���� ��Ŷ ����
				objects[old_one].send_out_packet(id);							//�þ߿��� ����� �÷��̾�� ������ �÷��̾� ���� ��Ŷ ����
			}
		}
		moved_client->view_list_mtx.lock();
		moved_client->view_list = new_view_list;										//�� ����Ʈ ����
		moved_client->view_list_mtx.unlock();
	}
	break;
	case CS_DIRECTION:
	{
		SESSION* watcher = &objects[id];
		CS_DIRECTION_PACKET* recv_packet = reinterpret_cast<CS_DIRECTION_PACKET*>(packet);
		
		if (watcher->player.direction == recv_packet->direction)
			return;

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
		
		if (attacker->prev_attack_time + PLAYER_ATTACK_TIME > recv_packet->time) return;	// �ð����� �Ÿ���
		attacker->prev_attack_time = recv_packet->time;
		//cout << attacker->id << " attack" << endl;

		int damage{};
		switch (recv_packet->attack_type)
		{
		case ATTACK_FORWARD:
			damage = FORWARD_ATTACK_DAMAGE;
			break;
		case ATTACK_WIDE:
			damage = WIDE_ATTACK_DAMAGE;
			break;
		default:
			cout << "Unknown Attack Type: process_packet()\n";
			break;
		}

		attacker->view_list_mtx.lock();
		unordered_set<int> attacker_view_list = attacker->view_list;
		attacker->view_list_mtx.unlock();
		
		//�¾Ҵ��� �ȸ¾Ҵ��� �Ǵ��ϰ� �׿� �´� ��Ŷ ������
		bool hit = false;
		bool dead = false;
		for (auto& watcher : attacker_view_list) {
			if (watcher >= MAX_USER + MAX_NPC) continue;	//��ֹ� ����
			if (attack_position(id, watcher, static_cast<ATTACK_TYPE>(recv_packet->attack_type))) {
				hit = true;
				dead = objects[watcher].player.decrease_hp(damage);

				if (get_object_type(watcher) == 1) {	//npc�� �¾����� Ÿ���� ���������� ����
					objects[watcher].enemy_id = id;
				}
				else if(!dead){	//player�� ���� ���� �ʾҴٸ� �� ȸ��
					natural_healing_start(watcher);
				}

				if (dead) {							//�׾��� ��
					int defender_level = objects[watcher].player.level;
					int bonus{1};
					if (watcher >= AGGR_NPC_START)
						bonus = 2;
					attacker->player.increase_exp(defender_level * defender_level * bonus);
					attacker->send_stat_packet();
					//disconnect(watcher);
					objects[watcher].dead();
				}
				objects[watcher].send_stat_packet();							//������ ����
			}
		}

		for (auto& watcher : attacker_view_list) {
			objects[watcher].send_attack_packet(id, hit, dead, static_cast<ATTACK_TYPE>(recv_packet->attack_type));		//���� ���
		}
		attacker->send_attack_packet(id, hit, dead, static_cast<ATTACK_TYPE>(recv_packet->attack_type));					//�´� �Ҹ�
	}
	break;
	case CS_LOGIN:
	{
		SESSION* new_client = &objects[id];
		CS_LOGIN_PACKET* recv_packet = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
		
		//player_count.fetch_add(1);
		//cout << "player count: " << player_count << endl;

		sql_mtx.lock();
		PI info = sql.find_by_name(recv_packet->name);
		sql_mtx.unlock();

		if (info.level == -1) {
			//cout << "���� ���� ����\n";
			// ���ο� ���� ����
			TI pos = random_spawn_location();
			sql_mtx.lock();
			sql.insert_new_account(recv_packet->name, 1, 0, 100, 100, pos.x, pos.y);
			info = sql.find_by_name(recv_packet->name);
			sql_mtx.unlock();
		}
		else {
			for (auto& playing_player : objects) {
				if (strcmp(playing_player.player.name, recv_packet->name) == 0) {	//�̹� �÷������� �÷��̾�
					objects[id].send_login_fail_packet();
					objects[id].player.level = -1;
					disconnect(id);
					return;
				}
			}
		}

		if (info.level == -1) {	//������ sql �����϶� ���� ����
			objects[id].send_login_fail_packet();
			objects[id].player.level = -1;
			disconnect(id);
			return;
		}

		// ���� ���� �ε�
		memcpy(&new_client->player.name, &recv_packet->name, sizeof(recv_packet->name));
		new_client->player.direction = DIR_DOWN;
		new_client->player.level = info.level;
		new_client->player.exp = info.exp;
		new_client->player.hp = info.hp;
		new_client->player.max_hp = info.max_hp;
		new_client->player.position.x = info.x;
		new_client->player.position.y = info.y;
		//cout << info.name << " " << info.level << " " << info.exp << " " << info.hp << " " << info.x << " " << info.y << endl;

		new_client->player.hp = new_client->player.max_hp;
		
		new_client->send_login_info_packet();										//���ο� ������ �α��� �ƴٰ� ����
		{
			lock_guard<mutex> m{ objects[id].state_mtx };
			objects[id].state = ST_INGAME;
		}

		new_client->add_to_sector_list();

		unordered_set<int> list_of_sector;
		new_client->get_from_sector_list(list_of_sector);

		for (auto& old_client_id : list_of_sector) {
			//{
			//	//lock_guard<mutex> m(old_client.state_mtx);
			//	if (old_client.state != ST_INGAME) continue;
			//}
			SESSION* old_client = &objects[old_client_id];
			if (id == old_client->id) continue;

			if (in_eyesight(id, old_client->id)) {
				
				old_client->insert_view_list(id);				//�þ� �ȿ� ���� Ŭ���� View list�� ���ο� �� �߰�
				new_client->insert_view_list(old_client->id);	//���ο� ���� viewlist�� �þ� �ȿ� ���� Ŭ�� �߰�
				
				new_client->send_in_packet(old_client->id);				//���� ���� Ŭ�󿡰� �þ� ���� ���� �ֵ� ��ġ ����
				old_client->send_in_packet(id);							//�þ� �ȿ� Ŭ������ ���ο� �� ��ġ ����.
				
				new_client->send_direction_packet(old_client->id);			//���� ���� Ŭ�󿡰� �þ� ���� ���� �ֵ� ���� ����
				old_client->send_direction_packet(id);						//�þ� �ȿ� Ŭ������ ���ο� �� ���� ����.
				
				if (get_object_type(old_client->id) == 1) {							//NPC��� �����
					wake_up_npc(old_client->id);
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
		unsigned short packet_size = packet[0];
		//cout << "client " << id << " : " << recv_packet->mess << " size:" << packet_size << endl;
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
		//�Ϸ�� ���¸� ������
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
			int data_to_process = num_bytes + objects[key].remain_data_size;
			memcpy(objects[key].process_field + objects[key].remain_data_size, ex_over->data, num_bytes);
			char* packet_ptr = objects[key].process_field;
			while (data_to_process > 0) {
				unsigned short packet_size = packet_ptr[0];
				if (packet_size <= data_to_process) {
					process_packet(static_cast<int>(key), packet_ptr);
					packet_ptr += packet_size;
					data_to_process -= packet_size;
				}
				else break;
			}
			objects[key].remain_data_size = data_to_process;
			if (data_to_process > 0) {
				//cout << "remain data size: " << data_to_process << endl;
				memcpy(objects[key].process_field, packet_ptr, data_to_process);
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
				objects[id].remain_data_size = 0;
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
		case OP_NATURAL_HEALING:
		{
			{
				lock_guard<mutex> m(objects[key].state_mtx);
				if (objects[key].state != ST_INGAME) break;
			}
			//cout << "Natural Heal" << endl;

			bool is_full = objects[key].player.natural_healing();
			objects[key].send_stat_packet();
			if (is_full) { 
				objects[key].is_natural_healing = false;
				break; 
			}
			reserve_timer(key, EV_NATURAL_HEALING_FOR_PLAYERS, NATURAL_HEALING_TIME);
			delete ex_over;
			break;
		}
		case OP_RESPAWN:
		{
			{
				lock_guard<mutex> m(objects[key].state_mtx);
				if (objects[key].state != ST_DEAD) break;
			}
			objects[key].respawn();
		delete ex_over;
		break;
		}
		case OP_NPC: 
		{
			EVENT_TYPE event_type = ex_over->event_type;
			do_npc(static_cast<int>(key), event_type);
			delete ex_over;
			break;
		}
		default: cout << "Unknown Completion Type" << endl; break;
		}
	}
}

int API_SendMessage(lua_State* L)
{
	int my_id = (int)lua_tointeger(L, -3);
	int user_id = (int)lua_tointeger(L, -2);
	char* mess = (char*)lua_tostring(L, -1);

	lua_pop(L, 4);
	objects[user_id].send_chat_packet(my_id, mess);
	return 0;
}

void spawn_npc()
{
	random_device rd;
	default_random_engine dre(rd());
	uniform_int_distribution <int>random_personality(0, 1);
	
	auto start_t = chrono::system_clock::now();
	for (int npc_id = MAX_USER; npc_id < MAX_USER + MAX_NPC; ++npc_id) {
		objects[npc_id].id = npc_id;
		objects[npc_id].state = ST_INGAME;
		objects[npc_id].player.position = random_spawn_location();
		objects[npc_id].enemy_id = -1;
		sprintf_s(objects[npc_id].player.name, "NPC %d", npc_id);
		objects[npc_id].add_to_sector_list();
		
		lua_State* L = objects[npc_id].lua = luaL_newstate();
		luaL_openlibs(L);
		luaL_loadfile(L, "npc.lua");
		int error = lua_pcall(L, 0, 0, 0);
		if (error) {
			cout << "Error:" << lua_tostring(L, -1);
			lua_pop(L, 1);
		}

		lua_getglobal(L, "set_id");
		lua_pushnumber(L, npc_id);
		lua_pcall(L, 1, 0, 0);
		lua_pop(L, 1);

		lua_register(L, "API_SendMessage", API_SendMessage);
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
	if (this_npc->player.hp <= 0)
		return;

	random_device rd;
	default_random_engine dre(rd());
	
	switch (event_type) {
	case EV_MOVE:
	{
		//cout << "NPC " << npc_id << " move" << endl;
		unordered_set<int> new_view_list;									//���� ������Ʈ �� �� ����Ʈ
		this_npc->view_list_mtx.lock();
		unordered_set<int> old_view_list = this_npc->view_list;			//���� �� ����Ʈ ����
		this_npc->view_list_mtx.unlock();

		if (old_view_list.size() == 0) {
			//cout << npc_id << " �ܴ� NPC\n";
			this_npc->is_active_npc = false;
			return;
		}
		
		uniform_int_distribution <int>random_direction(1, 8);
		this_npc->player.key_input = random_direction(dre);

		//�浹�˻�
		TI direction = key_to_dir(this_npc->player.key_input);
		if (collide_check({ direction.x + this_npc->player.position.x, direction.y + this_npc->player.position.y })) {
			reserve_timer(npc_id, EV_MOVE, NPC_MOVE_TIME);
			return;
		}
		//���� �ű��
		this_npc->move_sector();

		unordered_set<int> list_of_sector;
		this_npc->get_from_sector_list(list_of_sector);				//������ ���� ���Ϳ� �ִ� Ŭ���̾�Ʈ���� �ҷ���

		for (auto& client_in_sector : list_of_sector) {
			if (get_object_type(client_in_sector) != 0) continue;
			//{
			//	//lock_guard<mutex> m(objects[client_in_sector].state_mtx);
			//	if (objects[client_in_sector].state != ST_INGAME) continue;
			//}
			if (objects[client_in_sector].id == npc_id) continue;
			if (in_eyesight(npc_id, client_in_sector)) {							//���� �þ� �ȿ� �ִ� Ŭ���̾�Ʈ
				new_view_list.insert(client_in_sector);								//new list ä���
			}
		}

		for (auto& new_one : new_view_list) {
			if (old_view_list.count(new_one) == 0) {					//���� �þ߿� ���� �÷��̾�
				objects[new_one].insert_view_list(npc_id);		//�þ߿� ���� �÷��̾��� �� ����Ʈ�� ������ �÷��̾� �߰�
				objects[new_one].send_in_packet(npc_id);					//���� �þ߿� ���� �÷��̾�� ������ �÷��̾� ���� ����
			}
			else {
				objects[new_one].send_move_packet(npc_id);
			}
		}
		for (auto& old_one : old_view_list) {
			if (new_view_list.count(old_one) == 0) {					//�þ߿��� ����� �÷��̾�
				objects[old_one].send_out_packet(npc_id);			//�þ߿��� ����� �÷��̾�� ������ �÷��̾� ���� ��Ŷ ����
				objects[old_one].erase_view_list(npc_id);		//�þ߿��� ����� �÷��̾��� �� ����Ʈ���� ������ �÷��̾� ����
			}
		}  
		this_npc->view_list_mtx.lock();
		this_npc->view_list = new_view_list;								//�� ����Ʈ ����
		this_npc->view_list_mtx.unlock();

		if (new_view_list.size() == 0) {
			//cout << npc_id << " �ܴ� NPC\n";
			this_npc->is_active_npc = false;
			return;
		}
		//cout << npc_id << " moving\n";
		
		if (npc_id >= AGGR_NPC_START) {
			for (auto& player : new_view_list) {
				if (in_aggr_range(player, npc_id)) {
					this_npc->enemy_id = player;
					reserve_timer(npc_id, EV_FOLLOW, NPC_MOVE_TIME);
					return;
				}
			}
		}
		
		if (new_view_list.find(this_npc->enemy_id) != new_view_list.end()) {
			//cout << npc_id << " �� �߰�\n";
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
		uniform_int_distribution <int>random_attack_type(0, 1);
		ATTACK_TYPE attack_type = static_cast<ATTACK_TYPE>(random_attack_type(dre));
		int damage{};
		if (attack_type == ATTACK_FORWARD)
			damage = FORWARD_ATTACK_DAMAGE;
		else if (attack_type == ATTACK_WIDE)
			damage = WIDE_ATTACK_DAMAGE;
		else
			cout << "Unknown Attack Type: do_npc(); case: EV_ATTACK\n";

		if (!attack_position(npc_id, this_npc->enemy_id, attack_type)) {		//�Ÿ��� �Ǵµ� ������ �ȸ��� ���
			reserve_timer(npc_id, EV_DIRECTION, NPC_MOVE_TIME);
			return;
		}
		//cout << npc_id << " attack\n";
		unordered_set<int> attacker_view_list;
		unordered_set<int> list_of_sector;
		this_npc->get_from_sector_list( list_of_sector);
		for (auto& client_in_sector : list_of_sector) {
			//{
			//	//lock_guard<mutex> m(objects[client_in_sector].state_mtx);
			//	if (objects[client_in_sector].state != ST_INGAME) continue;
			//}
			if (client_in_sector >= MAX_USER + MAX_NPC) continue;
			if (client_in_sector == npc_id) continue;
			if (in_eyesight(npc_id, client_in_sector)) {						//���� �þ� �ȿ� �ִ� Ŭ���̾�Ʈ
				attacker_view_list.insert(client_in_sector);								//new list ä���
			}
		}

		//�¾Ҵ��� �ȸ¾Ҵ��� �Ǵ��ϰ� �׿� �´� ��Ŷ ������
		bool hit = false;
		bool dead = false;
		for (auto& watcher : attacker_view_list) {
			if (attack_position(npc_id, watcher, attack_type)) {
				hit = true;
				dead = objects[watcher].player.decrease_hp(damage);

				if (get_object_type(watcher) == 0 && !dead) {	//���� ���� player�϶� ���� �ʾҴٸ� �� ȸ��
					natural_healing_start(watcher);
				}
				//objects[watcher].enemy_id = npc_id;		//npc ������ �� ����

				if (dead) {							//�׾��� ��
					if(this_npc->enemy_id == watcher)
						this_npc->enemy_id = -1;
					
					int defender_level = objects[watcher].player.level;
					this_npc->player.increase_exp(defender_level * defender_level);
					//disconnect(watcher);
					objects[watcher].dead();

					//cout << "NPC�� " << watcher << " ����\n";
				}
				objects[watcher].send_stat_packet();							//������ ����
			}
		}

		for (auto& watcher : attacker_view_list) {
			objects[watcher].send_attack_packet(npc_id, hit, dead, attack_type);		//���� ���
		}
		
		if (attacker_view_list.find(this_npc->enemy_id) != attacker_view_list.end()) {		//���� ������ ��� ���󰡱�
			//cout << npc_id << " �� �߰�\n";
			reserve_timer(npc_id, EV_FOLLOW, NPC_MOVE_TIME);
			return;
		}

		this_npc->view_list_mtx.lock();
		unordered_set<int> old_view_list = this_npc->view_list;			//���� �� ����Ʈ ����
		this_npc->view_list_mtx.unlock();

		if (old_view_list.size() == 0) {
			//cout << npc_id << " �ܴ� NPC\n";
			this_npc->is_active_npc = false;
			return;
		}
		reserve_timer(npc_id, EV_MOVE, NPC_MOVE_TIME);
		return;
	}
	break;
	case EV_FOLLOW:
	{
		//cout << npc_id << " follow\n";
		unordered_set<int> new_view_list;									//���� ������Ʈ �� �� ����Ʈ
		this_npc->view_list_mtx.lock();
		unordered_set<int> old_view_list = this_npc->view_list;			//���� �� ����Ʈ ����
		this_npc->view_list_mtx.unlock();

		if (old_view_list.size() == 0) {
			//cout << npc_id << " �ܴ� NPC\n";
			this_npc->is_active_npc = false;
			return;
		}
		
		if (this_npc->enemy_id == -1) {					//���� ���ٸ�
			reserve_timer(npc_id, EV_MOVE, NPC_MOVE_TIME);
			return;
		}
		TI my_pos{ this_npc->player.position.x, this_npc->player.position.y };
		TI enemy_pos{ objects[this_npc->enemy_id].player.position.x, objects[this_npc->enemy_id].player.position.y };
		
		array<TI, SECTOR_SIZE* SECTOR_SIZE> obstacles;
		for (auto& obstacle : obstacles) obstacle = { -10000, -10000 };
		int obstacle_num = 0;

		unordered_set<int> list_of_sector_for_astar;
		this_npc->get_from_sector_list(list_of_sector_for_astar);
		for (auto& player_id : list_of_sector_for_astar) {
			if (get_object_type(player_id) != 2) continue;
			if (in_eyesight(npc_id, player_id)) {
				obstacles[obstacle_num] = { objects[player_id].player.position.x, objects[player_id].player.position.y };
				obstacle_num++;
			}
		}
		
		if (my_pos.x > enemy_pos.x) {
			for (auto& obstacle : obstacles)
				obstacle.x -= enemy_pos.x;
			my_pos.x -= enemy_pos.x;
			enemy_pos.x = 0;
		}
		else if (my_pos.x < enemy_pos.x) {
			for (auto& obstacle : obstacles)
				obstacle.x -= my_pos.x;
			enemy_pos.x -= my_pos.x;
			my_pos.x = 0;
		}
		else {
			for (auto& obstacle : obstacles)
				obstacle.x -= my_pos.x;
			my_pos.x = 0;
			enemy_pos.x = 0;
		}
		
		if (my_pos.y > enemy_pos.y) {
			for (auto& obstacle : obstacles)
				obstacle.y -= enemy_pos.y;
			my_pos.y -= enemy_pos.y;
			enemy_pos.y = 0;
		}
		else if (my_pos.y < enemy_pos.y) {
			for (auto& obstacle : obstacles)
				obstacle.y -= my_pos.y;
			enemy_pos.y -= my_pos.y;
			my_pos.y = 0;
		}
		else {
			for (auto& obstacle : obstacles)
				obstacle.y -= my_pos.y;
			my_pos.y = 0;
			enemy_pos.y = 0;
		}
		
		TI diff = { abs(enemy_pos.x - my_pos.x), abs(enemy_pos.y - my_pos.y) };
		if(diff.x + diff.y <= 1){		//���γ� ���� �� �پ��ִ� ��ġ
			//cout << npc_id << " ���� ����\n";
			reserve_timer(npc_id, EV_ATTACK, PLAYER_ATTACK_TIME);
			return;
		}
		
		TI offset;
		offset.x = VIEW_RANGE - diff.x / 2;
		offset.y = VIEW_RANGE - diff.y / 2;
		
		my_pos.x += offset.x;
		my_pos.y += offset.y;
		enemy_pos.x += offset.x;
		enemy_pos.y += offset.y;
		for (auto& obstacle : obstacles) {
			obstacle.x += offset.x;
			obstacle.y += offset.y;
		}
		
		TI next_pos = turn_astar(my_pos, enemy_pos, { SECTOR_SIZE, SECTOR_SIZE }, obstacles);
		
		this_npc->player.tc_direction = { 0, 0 };
		if (abs(next_pos.x - my_pos.x) + abs(next_pos.y - my_pos.y) <= 2) {
			if (my_pos.y > next_pos.y) 
				this_npc->player.tc_direction.y = -1;
			else if (my_pos.y < next_pos.y) 
				this_npc->player.tc_direction.y = 1;
			else if (my_pos.x > next_pos.x) 
				this_npc->player.tc_direction.x = -1;
			else if (my_pos.x < next_pos.x) 
				this_npc->player.tc_direction.x = 1;
		}
		else {
			if (my_pos.y > next_pos.y) 
				this_npc->player.tc_direction.y = -1;
			if (my_pos.y < next_pos.y) 
				this_npc->player.tc_direction.y = 1;
			if (my_pos.x > next_pos.x) 
				this_npc->player.tc_direction.x = -1;
			if (my_pos.x < next_pos.x) 
				this_npc->player.tc_direction.x = 1;
		}

		TI prev_pos = this_npc->player.position;
		int prev_x = prev_pos.x / SECTOR_SIZE;
		int prev_y = prev_pos.y / SECTOR_SIZE;
		this_npc->player.dir_check();
		TI curr_pos = this_npc->player.position;
		int curr_x = curr_pos.x / SECTOR_SIZE;
		int curr_y = curr_pos.y / SECTOR_SIZE;
		if (prev_x != curr_x || prev_y != curr_y) {
			this_npc->add_to_sector_list();
			this_npc->remove_from_sector_list( prev_pos);
		}

		//this_npc->prev_move_time = static_cast<unsigned>(duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count());

		unordered_set<int> list_of_sector;
		this_npc->get_from_sector_list( list_of_sector);				//������ ���� ���Ϳ� �ִ� Ŭ���̾�Ʈ���� �ҷ���

		for (auto& client_in_sector : list_of_sector) {
			if (get_object_type(client_in_sector) != 0) continue;
			//{
			//	//lock_guard<mutex> m(objects[client_in_sector].state_mtx);
			//	if (objects[client_in_sector].state != ST_INGAME) continue;
			//}
			if (objects[client_in_sector].id == npc_id) continue;
			if (in_eyesight(npc_id, client_in_sector)) {							//���� �þ� �ȿ� �ִ� Ŭ���̾�Ʈ
				new_view_list.insert(client_in_sector);								//new list ä���
			}
		}

		for (auto& new_one : new_view_list) {
			if (old_view_list.count(new_one) == 0) {					//���� �þ߿� ���� �÷��̾�
				objects[new_one].insert_view_list(npc_id);		//�þ߿� ���� �÷��̾��� �� ����Ʈ�� ������ �÷��̾� �߰�
				objects[new_one].send_in_packet(npc_id);					//���� �þ߿� ���� �÷��̾�� ������ �÷��̾� ���� ����
			}
			else {
				objects[new_one].send_move_packet(npc_id);
			}
		}
		for (auto& old_one : old_view_list) {
			if (new_view_list.count(old_one) == 0) {					//�þ߿��� ����� �÷��̾�
				objects[old_one].send_out_packet(npc_id);			//�þ߿��� ����� �÷��̾�� ������ �÷��̾� ���� ��Ŷ ����
				objects[old_one].erase_view_list(npc_id);		//�þ߿��� ����� �÷��̾��� �� ����Ʈ���� ������ �÷��̾� ����
			}
		}
		this_npc->view_list_mtx.lock();
		this_npc->view_list = new_view_list;								//�� ����Ʈ ����
		this_npc->view_list_mtx.unlock();

		if (new_view_list.size() == 0) {
			//cout << npc_id << " �ܴ� NPC\n";
			this_npc->is_active_npc = false;
			return;
		}

		if (new_view_list.find(this_npc->enemy_id) != new_view_list.end()) {		//���� ������ ��� ���󰡱�
			//cout << npc_id << " �� �߰�\n";
			reserve_timer(npc_id, EV_FOLLOW, NPC_MOVE_TIME);
			return;
		}
		reserve_timer(npc_id, EV_MOVE, NPC_MOVE_TIME);
		return;
	}
	case EV_SLEEP:
	{
		//cout << npc_id << " ���� NPC\n";
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

	cout << npc_id << " �̺�Ʈ ��\n";
	cout << event_type << endl;
}

void do_timer()
{
	EVENT event;
	int num_excuted_npc{};
	auto start_t = chrono::high_resolution_clock::now();
	
	while (1) {
		timer_mtx.lock();
		if (timer_queue.empty()) {										//�ƹ��͵� ������ ����
			timer_mtx.unlock();
			this_thread::sleep_for(1ms);
			continue;
		}
		else
			event = timer_queue.top();									//�� ���� �ִ°� ������
		timer_mtx.unlock();
		
		if (event.exec_time > chrono::system_clock::now()) {			//���� �ð����� ũ�� 
			this_thread::sleep_for(1ms);						//�� ��ٸ���
			continue;
		}
		
		timer_mtx.lock();
		timer_queue.pop();
		timer_mtx.unlock();
		
		OVER_EXP* over = new OVER_EXP();
		over->event_type = event.type;
		if (EV_NATURAL_HEALING_FOR_PLAYERS == over->event_type)
			over->completion_type = OP_NATURAL_HEALING;
		else if (EV_RESPAWN == over->event_type)
			over->completion_type = OP_RESPAWN;
		else
			over->completion_type = OP_NPC;
		
		PostQueuedCompletionStatus(h_iocp, 1, event.object_id, &over->over);
		
		//cout << event.object_id << " position : " << objects[event.object_id].player.position.x << ", " << objects[event.object_id].player.position.y << " event : " << event.type << endl;
		//++num_excuted_npc;
		//if (chrono::high_resolution_clock::now() - start_t > chrono::milliseconds(NPC_MOVE_TIME)) {
		//	/*cout << num_excuted_npc << "���� NPC�� ������\n";
		//	num_excuted_npc = 0;*/
		//	show_all_sector_list();

		//	start_t = chrono::high_resolution_clock::now();
		//}
		
	}
}

void locate_obstacles()
{
	auto start_t = chrono::system_clock::now();
	for (int obstacle_id = MAX_USER + MAX_NPC; obstacle_id < MAX_USER + MAX_NPC + MAX_OBSTACLE; ++obstacle_id) {
		objects[obstacle_id].id = obstacle_id;
		//objects[obstacle_id].state = ST_INGAME;
		objects[obstacle_id].player.position = random_spawn_location();
		objects[obstacle_id].add_to_sector_list();
	}
	auto end_t = chrono::system_clock::now();
	cout << "��ֹ� ��ġ �Ϸ� : " << chrono::duration_cast<chrono::milliseconds>(end_t - start_t).count() << "ms\n";
}

int main()
{
	//sql.delete_all();
	//sql.show_all();
	
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
	//IOCP ���� (������ ���� 0�� �ھ� ������ŭ ���)
	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	//���� IOCP �����Ѱ� ExistingCompletionPort�� �־���. key�� ���Ƿ� �ƹ��ų�. ������ ���� ����
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(global_server_socket), h_iocp, 9999, 0);
	global_client_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	global_accept_over.completion_type = OP_ACCEPT;
	AcceptEx(global_server_socket, global_client_socket, global_accept_over.data, 0, addr_size + 16, addr_size + 16, 0, &global_accept_over.over);
	
	initialize_sector_list();
	locate_obstacles();
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
