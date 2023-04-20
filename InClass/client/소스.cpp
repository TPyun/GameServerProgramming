////클라
//#include <SFML/Graphics.hpp>
//#include <SFML/Network.hpp>
//#include <iostream>
//using namespace std;
//
//#pragma comment (lib, "opengl32.lib")
//#pragma comment (lib, "winmm.lib")
//#pragma comment (lib, "ws2_32.lib")
//
//#include "../protocol.h"
//
//sf::TcpSocket socket;
//
//constexpr auto SCREEN_WIDTH = W_WIDTH;
//constexpr auto SCREEN_HEIGHT = W_HEIGHT;
//
//constexpr auto TILE_WIDTH = 65;
//constexpr auto WINDOW_WIDTH = SCREEN_WIDTH * TILE_WIDTH;   // size of window
//constexpr auto WINDOW_HEIGHT = SCREEN_WIDTH * TILE_WIDTH;
//constexpr auto MAX_USER = 10;
//
//int g_left_x;
//int g_top_y;
//int g_myid;
//
//sf::RenderWindow* g_window;
//sf::Font* g_font;
//
//class OBJECT {
//private:
//	bool m_showing;
//	sf::Sprite m_sprite;
//	sf::Text m_name;
//public:
//	int m_x, m_y;
//	char name[NAME_SIZE];
//	OBJECT(sf::Texture& t, int x, int y, int x2, int y2) {
//		m_showing = false;
//		m_sprite.setTexture(t);
//		m_sprite.setTextureRect(sf::IntRect(x, y, x2, y2));
//	}
//	OBJECT() {
//		m_showing = false;
//	}
//	void show()
//	{
//		m_showing = true;
//	}
//	void hide()
//	{
//		m_showing = false;
//	}
//
//	void a_move(int x, int y) {
//		m_sprite.setPosition((float)x, (float)y);
//	}
//
//	void a_draw() {
//		g_window->draw(m_sprite);
//	}
//
//	void move(int x, int y) {
//		m_x = x;
//		m_y = y;
//	}
//	void draw() {
//		if (false == m_showing) return;
//		float rx = (m_x) * 65.0f + 1;
//		float ry = (m_y) * 65.0f + 1;
//		m_sprite.setPosition(rx, ry);
//		g_window->draw(m_sprite);
//		auto size = m_name.getGlobalBounds();
//		m_name.setPosition(rx + 32 - size.width / 2, ry - 10);
//		g_window->draw(m_name);
//	}
//
//	void set_name(const char str[]) {
//		m_name.setFont(*g_font);
//		m_name.setString(str);
//		m_name.setCharacterSize(20);
//		m_name.setFillColor(sf::Color(255, 255, 0));
//		m_name.setStyle(sf::Text::Bold);
//	}
//};
//
//OBJECT avatar;
//std::string avatar_name;
//OBJECT players[MAX_USER];
//
//OBJECT white_tile;
//OBJECT black_tile;
//
//sf::Texture* board;
//sf::Texture* pieces;
//
//void client_initialize()
//{
//	board = new sf::Texture;
//	pieces = new sf::Texture;
//	g_font = new sf::Font;
//	board->loadFromFile("chessmap.bmp");
//	pieces->loadFromFile("chess2.png");
//	if (false == g_font->loadFromFile("cour.ttf")) {
//		cout << "Font Loading Error!\n";
//		exit(-1);
//	}
//	white_tile = OBJECT{ *board, 5, 5, TILE_WIDTH, TILE_WIDTH };
//	black_tile = OBJECT{ *board, 69, 5, TILE_WIDTH, TILE_WIDTH };
//	avatar = OBJECT{ *pieces, 128, 0, 64, 64 };
//	avatar.set_name(avatar_name.c_str());
//	avatar.move(4, 4);
//	for (auto& pl : players) {
//		pl = OBJECT{ *pieces, 64, 0, 64, 64 };
//	}
//}
//
//void client_finish()
//{
//	delete g_font;
//	delete board;
//	delete pieces;
//}
//
//void ProcessPacket(char* ptr)
//{
//	static bool first_time = true;
//	switch (ptr[1])
//	{
//	case SC_LOGIN_INFO:
//	{
//		SC_LOGIN_INFO_PACKET* packet = reinterpret_cast<SC_LOGIN_INFO_PACKET*>(ptr);
//		g_myid = packet->id;
//		avatar.m_x = packet->x;
//		avatar.m_y = packet->y;
//		avatar.show();
//	}
//	break;
//
//	case SC_ADD_PLAYER:
//	{
//		SC_ADD_PLAYER_PACKET* my_packet = reinterpret_cast<SC_ADD_PLAYER_PACKET*>(ptr);
//		int id = my_packet->id;
//
//		if (id == g_myid) {
//			avatar.move(my_packet->x, my_packet->y);
//			g_left_x = my_packet->x - 4;
//			g_top_y = my_packet->y - 4;
//			avatar.show();
//		}
//		else if (id < MAX_USER) {
//			players[id].move(my_packet->x, my_packet->y);
//			players[id].set_name(my_packet->name);
//			players[id].show();
//		}
//		else {
//			//npc[id - NPC_START].x = my_packet->x;
//			//npc[id - NPC_START].y = my_packet->y;
//			//npc[id - NPC_START].attr |= BOB_ATTR_VISIBLE;
//		}
//		break;
//	}
//	case SC_MOVE_PLAYER:
//	{
//		SC_MOVE_PLAYER_PACKET* my_packet = reinterpret_cast<SC_MOVE_PLAYER_PACKET*>(ptr);
//		int other_id = my_packet->id;
//		if (other_id == g_myid) {
//			avatar.move(my_packet->x, my_packet->y);
//			g_left_x = my_packet->x - 4;
//			g_top_y = my_packet->y - 4;
//		}
//		else if (other_id < MAX_USER) {
//			players[other_id].move(my_packet->x, my_packet->y);
//		}
//		else {
//			//npc[other_id - NPC_START].x = my_packet->x;
//			//npc[other_id - NPC_START].y = my_packet->y;
//		}
//		break;
//	}
//
//	case SC_REMOVE_PLAYER:
//	{
//		SC_REMOVE_PLAYER_PACKET* my_packet = reinterpret_cast<SC_REMOVE_PLAYER_PACKET*>(ptr);
//		int other_id = my_packet->id;
//		if (other_id == g_myid) {
//			avatar.hide();
//		}
//		else if (other_id < MAX_USER) {
//			players[other_id].hide();
//		}
//		else {
//			//		npc[other_id - NPC_START].attr &= ~BOB_ATTR_VISIBLE;
//		}
//		break;
//	}
//	default:
//		printf("Unknown PACKET type [%d]\n", ptr[1]);
//	}
//}
//
//void process_data(char* net_buf, size_t io_byte)
//{
//	char* ptr = net_buf;
//	static size_t in_packet_size = 0;
//	static size_t saved_packet_size = 0;
//	static char packet_buffer[BUF_SIZE];
//
//	while (0 != io_byte) {
//		if (0 == in_packet_size) in_packet_size = ptr[0];
//		if (io_byte + saved_packet_size >= in_packet_size) {
//			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
//			ProcessPacket(packet_buffer);
//			ptr += in_packet_size - saved_packet_size;
//			io_byte -= in_packet_size - saved_packet_size;
//			in_packet_size = 0;
//			saved_packet_size = 0;
//		}
//		else {
//			memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
//			saved_packet_size += io_byte;
//			io_byte = 0;
//		}
//	}
//}
//
//void client_main()
//{
//	char net_buf[BUF_SIZE];
//	size_t	received;
//
//	auto recv_result = socket.receive(net_buf, BUF_SIZE, received);
//	if (recv_result == sf::Socket::Error)
//	{
//		wcout << L"Recv 에러!";
//		while (true);
//	}
//	if (recv_result != sf::Socket::NotReady)
//		if (received > 0) process_data(net_buf, received);
//
//	for (int i = 0; i < SCREEN_WIDTH; ++i)
//		for (int j = 0; j < SCREEN_HEIGHT; ++j)
//		{
//			if (0 == (i + j) % 2) {
//				white_tile.a_move(TILE_WIDTH * i, TILE_WIDTH * j);
//				white_tile.a_draw();
//			}
//			else
//			{
//				black_tile.a_move(TILE_WIDTH * i, TILE_WIDTH * j);
//				black_tile.a_draw();
//			}
//		}
//	avatar.draw();
//	for (auto& pl : players) pl.draw();
//}
//
//void send_packet(void* packet)
//{
//	unsigned char* p = reinterpret_cast<unsigned char*>(packet);
//	size_t sent = 0;
//	socket.send(packet, p[0], sent);
//}
//
//int main()
//{
//	wcout.imbue(locale("korean"));
//	std::cout << "Enter User Name : ";
//	std::cin >> avatar_name;
//	sf::Socket::Status status = socket.connect("127.0.0.1", PORT_NUM);
//	socket.setBlocking(false);
//
//	if (status != sf::Socket::Done) {
//		wcout << L"서버와 연결할 수 없습니다.\n";
//		while (true);
//	}
//
//	CS_LOGIN_PACKET p;
//	p.size = sizeof(p);
//	p.type = CS_LOGIN;
//	strcpy_s(p.name, avatar_name.c_str());
//	send_packet(&p);
//
//
//	client_initialize();
//
//	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "2D CLIENT");
//	g_window = &window;
//
//	while (window.isOpen())
//	{
//		sf::Event event;
//		while (window.pollEvent(event))
//		{
//			if (event.type == sf::Event::Closed)
//				window.close();
//			if (event.type == sf::Event::KeyPressed) {
//				int direction = -1;
//				switch (event.key.code) {
//				case sf::Keyboard::Left:
//					direction = 2;
//					break;
//				case sf::Keyboard::Right:
//					direction = 3;
//					break;
//				case sf::Keyboard::Up:
//					direction = 0;
//					break;
//				case sf::Keyboard::Down:
//					direction = 1;
//					break;
//				case sf::Keyboard::Escape:
//					window.close();
//					break;
//				}
//				if (-1 != direction) {
//					CS_MOVE_PACKET p;
//					p.size = sizeof(p);
//					p.type = CS_MOVE;
//					p.direction = direction;
//					send_packet(&p);
//				}
//			}
//		}
//
//		window.clear();
//		client_main();
//		window.display();
//	}
//	client_finish();
//
//	return 0;
//}
//
////#include <thread>
////#include <iostream>
////#include <vector>
////#include <Windows.h>
////using namespace std;
////
////void ThreadFunc(int threadid)
////{
////	while (1) {
////		cout << threadid << endl;
////		Sleep(10);
////	};
////}
////
////int main()
////{
////	vector <thread> my_thread;
////	
////	for (int i = 0; i < 10; ++i)
////		my_thread.emplace_back(ThreadFunc, i);
////	
////	
////	for (auto& t : my_thread) 
////		t.join();
////}
//
//
////
////#include <iostream>
////#include <thread>
////#include <chrono>
////#include <vector>
////#include <mutex>
////#include <Windows.h>
////
////constexpr int MAX_THREADS = 256;
////using namespace std;
////using namespace std::chrono;
////volatile int sum;
////mutex mtx;
////
////struct NUM {
////	alignas(64) volatile int sum;
////};
////NUM num[MAX_THREADS];
////
////void thread_func(int num_threads, int thread_id) {
////	for (auto i = 0; i < 10000000 / num_threads; ++i) {
////		num[thread_id].sum++;
////	}
////}
////int main() {
////	vector<thread> threads;
////	for (auto i = 1; i <= MAX_THREADS; i *= 2) {
////		for (auto n : num) {
////			n.sum = 0;
////		}
////		
////		threads.clear();
////		auto start = high_resolution_clock::now();
////		for (auto j = 0; j < i; ++j) 
////			threads.push_back(thread{ thread_func, i, j});
////
////		for (auto& tmp : threads) 
////			tmp.join();
////
////		auto duration = high_resolution_clock::now() - start;
////
////		for (auto n : num) {
////			sum += n.sum;
////		}
////		cout << i << " Threads, Sum = " << sum;
////		cout << " Duration = " << duration_cast<milliseconds>(duration).count() << " milliseconds\n";
////	}
////}
