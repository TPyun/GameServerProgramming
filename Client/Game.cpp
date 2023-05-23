#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include "Game.h"

using namespace std;
using namespace chrono;

Game::Game()
{
	sfml_window = new sf::RenderWindow(sf::VideoMode(WIDTH, HEIGHT), "SFML window");
	if (sfml_window == NULL) {
		cout << "Could not create SFML window!" << endl;
	}
	if (!sfml_font.loadFromFile("arial.ttf")) {
		cout << "Could not load font!" << endl;
	}
	
	//draw_sfml_text_s(TI{ WIDTH / 2 - players[my_id].size.x / 2 - 90 , HEIGHT / 2 - players[my_id].size.y / 2 }, "Loading Game", sf::Color::White, 30);
	//render();
	
	char player_tex_file[6][20]{ "Idle.png", "Walk.png", "Run.png", "Push.png", "Attack.png", "Hit.png" };
	for (int act = 0; act < 6; act++) {
		char player_tex_root[30] = "Texture/Player/";
		if (!player_texture[act].loadFromFile(strcat(player_tex_root, player_tex_file[act])))
			cout << "Image not loaded!" << endl;
		else {
			player_sprite[act].setTexture(player_texture[act]);
		}
	}

	char sound_file[6][20]{ "yell.wav", "hit.wav", "attack_air.wav", "walk_grass.wav", "env.wav",  "turn_on.wav"};
	for (int type = 0; type < 6; type++) {
		char sounds_root[30] = "Sounds/";
		if (!sound_buffer[type].loadFromFile(strcat(sounds_root, sound_file[type])))
			cout << "Sound not loaded!" << endl;
		else {
			sounds[type].setBuffer(sound_buffer[type]);
			sounds[type].setVolume(10);
		}
	}
	sounds[SOUND_MOVE].setLoop(true);
	sounds[SOUND_TURN_ON].play();
	
	sfml_window->setFramerateLimit(set_fps);
	cout << "Press Tab to move another input box" << endl;
	cout << "Press Enter to connect" << endl;
}

Game::~Game()
{
	sfml_window->close();
	delete sfml_window;
}

void Game::update()
{
	bool input = sfml_window->pollEvent(sfml_event);
	if (input){
		if (sfml_event.type == sf::Event::Closed || (sfml_event.type == sf::Event::KeyPressed && sfml_event.key.code == sf::Keyboard::Escape)) {
			if (scene == 0) {
				//std::cout << "Game closed!" << std::endl;
				isRunning = false;
				return;
			}
			if (chat_mode) {
				chat_mode = false;
				ZeroMemory(chat_message, sizeof(chat_message));
				text_input = "";
				return;
			}
			if (scene == 1) {
				//cout << "Disconnected!" << endl;
				connected = false;
				return;
			}
		}
	}

	if (scene == 0) {
		if (input)
			main_handle_events();
		draw_main();
	}
	else if (scene == 1) {
		if (input && chat_mode)
			chat_mode_handle_events();
		else if (input)
			game_handle_events();
		
		players_mtx.lock();
		draw_game();
		timer();
		players_mtx.unlock();
	}
}

template<typename T>
T lerp(const T& a, const T& b, float t) {
	return a + (b - a) * t;
}

void Game::timer()
{
	unsigned int current_time = static_cast<unsigned>(duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count());
	for (auto& player : players) {
		if (current_time - player.second.sprite_time > 100) {
			player.second.sprite_iter++;
			player.second.sprite_time = current_time;
			//cout << "Sprite iter: " << (int)player.second.sprite_iter << endl;
		}
		
		if (player.second.chat_time != 0) {
			if (player.second.chat_time + 5000 < current_time) {
				player.second.chat_time = 0;
				player.second.chat.clear();
			}
		}

		if (abs(player.second.arr_position.x - player.second.curr_position.x) < 0.05f && abs(player.second.arr_position.y - player.second.curr_position.y) < 0.05f) {
			player.second.curr_position.x = player.second.arr_position.x;
			player.second.curr_position.y = player.second.arr_position.y;
			player.second.state = ST_IDLE;
		}
		//lerp position in same velocity with time
		else {
			//player.second.curr_position.x = lerp(player.second.curr_position.x, (float)player.second.arr_position.x, 1.f / (float)real_fps * 3);
			//player.second.curr_position.y = lerp(player.second.curr_position.y, (float)player.second.arr_position.y, 1.f / (float)real_fps * 3);
			char sign_x = -1;
			char sign_y = -1;
			if (player.second.arr_position.x - player.second.curr_position.x == 0)
				sign_x = 0;
			else if (player.second.arr_position.x - player.second.curr_position.x > 0)
				sign_x = 1;
			if (player.second.arr_position.y - player.second.curr_position.y == 0)
				sign_y = 0;
			else if (player.second.arr_position.y - player.second.curr_position.y > 0)
				sign_y = 1;
			
			float delay = 4.f;
			float velocity = delay / (float)real_fps;
			player.second.curr_position.x += (float)sign_x * velocity;
			player.second.curr_position.y += (float)sign_y * velocity;
			
			player.second.state = ST_MOVE;
		}
	
		if (player.second.attack_time + 300 > current_time) {
			player.second.state = ST_ATTACK;
		}
	}
}

void Game::render()
{
	sfml_window->display();
}

void Game::clear()
{
	sfml_window->clear(sf::Color::Black);
}

void Game::draw_main()
{
	draw_sfml_rect(TI{ 99, input_height}, TI{200, 20}, sf::Color(80, 80, 80), sf::Color(80, 80, 80));

	draw_sfml_rect(TI{ 99, 130 }, TI{ 200, 20 }, sf::Color::White, sf::Color::Transparent);
	draw_sfml_rect(TI{ 99, 230 }, TI{ 200, 20 }, sf::Color::White, sf::Color::Transparent);
	draw_sfml_rect(TI{ 99, 330 }, TI{ 200, 20 }, sf::Color::White, sf::Color::Transparent);
	draw_sfml_rect(TI{ 99, 430 }, TI{ 200, 20 }, sf::Color::White, sf::Color::Transparent);
	
	draw_sfml_text(TI{ 60, input_height }, (char*)"Tab", sf::Color(200, 200, 200), 17);
	
	draw_sfml_text(TI{ 100, 100 }, (char*)"IP Address", sf::Color(200, 200, 200), 17);
	draw_sfml_text(TI{ 100, 200 }, (char*)"Port", sf::Color(200, 200, 200), 17);
	draw_sfml_text(TI{ 100, 300 }, (char*)"Name", sf::Color(200, 200, 200), 17);
	draw_sfml_text(TI{ 100, 400 }, (char*)"Play Game", sf::Color(200, 200, 200), 17);
	
	draw_sfml_text(TI{ 100, 130 }, (char*)ip_address, sf::Color(200, 200, 200), 17);
	draw_sfml_text(TI{ 100, 230 }, (char*)Port, sf::Color(200, 200, 200), 17);
	draw_sfml_text(TI{ 100, 330 }, (char*)Name, sf::Color(200, 200, 200), 17);
	draw_sfml_text(TI{ 100, 430 }, (char*)"Press Enter", sf::Color(200, 200, 200), 17);
	
	if(input_warning)
		draw_sfml_text(TI{ 100, 500 }, (char*)"Please Fill All Fields", sf::Color(200, 0, 0), 17);
}

void Game::main_handle_events()
{
	if (sfml_event.type == sf::Event::KeyPressed && sfml_event.key.code == sf::Keyboard::Backspace && text_input.size()) {
		//cout << "Back" << endl;
		text_input.pop_back();
		switch (input_height) {
		case 130:
			strcpy(ip_address, text_input.c_str());
			break;
		case 230:
			strcpy(Port, text_input.c_str());
			break;
		case 330:
			strcpy(Name, text_input.c_str());
			break;
		}
	}
	else if (sfml_event.type == sf::Event::KeyPressed && sfml_event.key.code == sf::Keyboard::Tab) {
		//cout << "Tab" << endl;
		switch (input_height) {
		case 130:
			input_height += 100;
			text_input = Port;
			break;
		case 230:
			input_height += 100;
			text_input = Name;
			break;
		case 330:
			input_height += 100;
			break;
		case 430:
			input_height = 130;
			text_input = ip_address;
			break;
		}
	}
	else if (sfml_event.type == sf::Event::KeyPressed && sfml_event.key.code == sf::Keyboard::Return && input_height == 430) {
		if (strlen(ip_address) && strlen(Port) && strlen(Name))
			try_connect = true;
		else
			input_warning = true;
	}
	else if (sfml_event.type == sf::Event::TextEntered && text_input.size() < 20) {
		//cout << "Typing: " << static_cast<char>(sfml_event.text.unicode) << endl;
		// Only add ASCII characters
		if (static_cast<char>(sfml_event.text.unicode) > 30) {		//지우는거랑 엔터 안먹히게 제한
			text_input += static_cast<char>(sfml_event.text.unicode);
			
			switch (input_height) {
			case 130:
				strcpy(ip_address, text_input.c_str());
				break;
			case 230:
				strcpy(Port, text_input.c_str());
				break;
			case 330:
				strcpy(Name, text_input.c_str());
				break;
			}
		}
	}
}

TI Game::get_relative_location(TI position)
{
	TI relative_position = { (WIDTH / 2) + (position.x - players[my_id].curr_position.x) * BLOCK_SIZE, (HEIGHT / 2) + (position.y - players[my_id].curr_position.y) * BLOCK_SIZE };
	return relative_position;
}

TI Game::get_relative_location(TF position)
{
	TI relative_position = { (WIDTH / 2) + (position.x - players[my_id].curr_position.x) * BLOCK_SIZE, (HEIGHT / 2) + (position.y - players[my_id].curr_position.y) * BLOCK_SIZE };
	return relative_position;
}

void Game::initialize_main()
{
	stop_sound(SOUND_ENV);
	play_sound(SOUND_TURN_ON);
	
	input_height = 130;
	text_input = "";
	input_warning = false;
	scene = 0;
}

void Game::initialize_ingame()
{
	play_sound(SOUND_ENV);
	stop_sound(SOUND_TURN_ON);
	
	information_mode = false;
	chat_mode = false;
	scene = 1;
	cout << "You can move with direction keys." << endl;
	cout << "Press Tab to see information." << endl;
}

void Game::play_sound(char sound)
{
	sounds[sound].play();
}

void Game::stop_sound(char sound)
{
	sounds[sound].stop();
}

void Game::draw_game()
{
	TF player_position = players[my_id].curr_position;
	
	//체스 판
	TI chess_board_pixel_size{ BLOCK_SIZE, BLOCK_SIZE };
	TI chess_board_pixel_position;
	for (int i = 0; i < CLIENT_RANGE; i++) {
		for (int j = 0; j < CLIENT_RANGE; j++) {
			chess_board_pixel_position.x = i * BLOCK_SIZE;
			chess_board_pixel_position.y = j * BLOCK_SIZE;
			sf::Color color(100, 50, 0);
			
			if (i == CLIENT_RANGE / 2 && j == CLIENT_RANGE / 2)
				color = sf::Color(255, 255, 255);
			else if (i == CLIENT_RANGE / 2 || j == CLIENT_RANGE / 2)
				color = sf::Color(200, 100, 0);

			draw_sfml_rect(chess_board_pixel_position, chess_board_pixel_size, color, color);
		}
	}
	
	// Sort the players by their Y position using qsort and a lambda function
	std::vector<std::pair<int, Player>> sorted_players(players.begin(), players.end());
	
	qsort(sorted_players.data(), sorted_players.size(), sizeof(std::pair<int, Player>), [](const void* a, const void* b) {
		const auto& player1 = *reinterpret_cast<const std::pair<int, Player>*>(a);
		const auto& player2 = *reinterpret_cast<const std::pair<int, Player>*>(b);
		return player1.second.arr_position.y - player2.second.arr_position.y;
	});

	// Draw the players in sorted order
	for (const auto& player : sorted_players) {
		draw_sprite(player.first, sf::Color::White, 3); // Walk
		TI related_pos = get_relative_location(player.second.curr_position);
		string name(player.second.name);
		draw_sfml_text_s({ related_pos.x - (int)name.length() * 4, related_pos.y - 80}, name, sf::Color::White, 14);

		if (player.second.chat_time) {		//draw chat
			draw_sfml_text_s({ related_pos.x - (int)player.second.chat.length() * 4, related_pos.y - 100 }, player.second.chat, sf::Color::White, 14);
		}
	}
	
	draw_stat();
	if(information_mode)
		draw_information_mode();
	if (chat_mode) 
		draw_chat_mode();
}

void Game::draw_stat()
{
	//draw hp, level, exp gauge
	draw_sfml_text_s({ 10, 10 }, "HP", sf::Color::White, 15);
	draw_sfml_text_s({ 10, 25 }, "EXP", sf::Color::White, 15);
	draw_sfml_text_s({ 10, 40 }, "Level: " + to_string(players[my_id].level), sf::Color::White, 15);

	int hp = players[my_id].hp;
	int max_hp = players[my_id].max_hp;
	int level = players[my_id].level;
	int exp = players[my_id].exp;
	
	int hp_gauge = (int)((float)hp * 200.f / max_hp);
	int exp_gauge = ((float)exp * 2 / level);
	
	draw_sfml_rect({ 50, 10 }, { hp_gauge, 15 }, sf::Color::Transparent, sf::Color::Red);
	draw_sfml_rect({ 50, 25 }, { (int)exp_gauge, 15 }, sf::Color::Transparent, sf::Color::Green);
	
	draw_sfml_rect({ 50, 10 }, { 200, 30 }, sf::Color::White, sf::Color::Transparent);
}

void Game::draw_sprite(int id, sf::Color color, char size)
{
	Player* this_player = &players[id];
	
	TI position;
	if (id == my_id)
		position = TI{ WIDTH / 2, HEIGHT / 2 };
	else
		position = get_relative_location(this_player->curr_position);
	TUC sprite_size;
	char sprite_length = 4;
	Direction direction = this_player->direction;
	unsigned char sprite_i = this_player->sprite_iter;
	
	player_sprite[this_player->state].setColor(color);
	player_sprite[this_player->state].setScale(size, size);

	switch (this_player->state) {
	case ST_MOVE:
	case ST_IDLE:
		sprite_size = { 16, 24 };
		break;
	case ST_ATTACK:
		sprite_size = { 24, 24 };
		break;
	}

	player_sprite[this_player->state].setPosition(position.x - (sprite_size.x * size) / 2 - 1, position.y - (sprite_size.y * size));
	player_sprite[this_player->state].setTextureRect(sf::IntRect(sprite_i % sprite_length * sprite_size.x, sprite_size.y * (direction + 1), sprite_size.x, sprite_size.y));
	sfml_window->draw(player_sprite[this_player->state]);
}

void Game::draw_information_mode()
{
	//draw player list
	int text_size = 13;
	draw_sfml_rect(TI{ 10, 10 }, TI{ 130, 730 }, sf::Color(150, 150, 150), sf::Color(150, 150, 150, 200));
	int information_height = 0;
	draw_sfml_text_s(TI{ 15, 10 + information_height++ * (text_size + 2) }, "Ping: " + std::to_string(ping), sf::Color::Black, text_size);
	draw_sfml_text_s(TI{ 15, 10 + information_height++ * (text_size + 2) }, std::to_string(my_id) + ": " +  std::to_string(players[my_id].curr_position.x) + ", " + std::to_string(players[my_id].curr_position.y), sf::Color::Black, text_size);

	for (auto& player : players) {
		if (player.first == my_id)
			continue;
		draw_sfml_text_s(TI{ 15, 10 + information_height++ * (text_size + 2) }, std::to_string(player.first) + ": " + std::to_string(player.second.curr_position.x) + ", " + std::to_string(player.second.curr_position.y), sf::Color::Red, text_size);
	}

	//draw map
	int minimap_frame_size = 590;
	TI minimap_frame_start_point{ 150, 10 };
	int minimap_offset = 50;
	TI minimap_start_point{ minimap_frame_start_point.x + minimap_offset / 2, minimap_frame_start_point.y + minimap_offset  / 2};
	TI minimap_size{ minimap_frame_size - minimap_offset, minimap_frame_size - minimap_offset };
	draw_sfml_rect(minimap_frame_start_point, TI{ minimap_frame_size, minimap_frame_size }, sf::Color(150, 150, 150), sf::Color(150, 150, 150, 200));	//mini map frame
	draw_sfml_rect(minimap_start_point, minimap_size, sf::Color::Black, sf::Color::Black);	//mini map
	
	for (auto& player : players) {
		TI player_pos_minimap{ player.second.curr_position.x * minimap_size.x / MAP_SIZE + minimap_start_point.x, player.second.curr_position.y * minimap_size.y / MAP_SIZE + minimap_start_point.y };
		if (player.first == my_id)
			continue;
		draw_sfml_rect(player_pos_minimap, TI{ 1, 1 }, sf::Color::Red, sf::Color::Red);	//other position on mini map
	}
	TI player_pos_minimap{ players[my_id].curr_position.x * minimap_size.x / MAP_SIZE + minimap_start_point.x,  players[my_id].curr_position.y * minimap_size.y / MAP_SIZE + minimap_start_point.y};
	draw_sfml_rect(player_pos_minimap, TI{ 1, 1 }, sf::Color::White, sf::Color::White);	//my position on mini map
}

void Game::draw_chat_mode()
{
	draw_sfml_rect(TI{ 10, HEIGHT - 30 }, TI{ 400, 20 }, sf::Color(150, 150, 150), sf::Color(150, 150, 150, 200));
	draw_sfml_text(TI{ 15, HEIGHT - 30 }, (char*)chat_message, sf::Color::Black, 17);
}

void Game::chat_mode_handle_events()
{
	if (chat_start) {	//채팅 시작할때 t버튼 먹는거 제거
		chat_start = false;
		ZeroMemory(chat_message, sizeof(chat_message));
		text_input = "";
		return;
	}
	
	if (sfml_event.type == sf::Event::KeyPressed && sfml_event.key.code == sf::Keyboard::Backspace && text_input.size()) {
		//지우기
		text_input.pop_back();
		strcpy(chat_message, text_input.c_str());
	}
	if (sfml_event.type == sf::Event::KeyPressed && sfml_event.key.code == sf::Keyboard::Return) {
		//전송
		chat_flag = true;
		chat_mode = false;
		cout << "chat message: " << chat_message << endl;
	}
	else if (sfml_event.type == sf::Event::TextEntered && text_input.size() < 50) {
		// Only add ASCII characters
		if (static_cast<char>(sfml_event.text.unicode) > 30) {		//지우는거랑 엔터 안먹히게 제한
			text_input += static_cast<char>(sfml_event.text.unicode);
			strcpy(chat_message, text_input.c_str());
		}
	}
}

void Game::game_handle_events()
{
	if (sfml_event.type == sf::Event::KeyPressed) {
		if (!move_flag) {
			bool move = false;
			if (sfml_event.key.code == sf::Keyboard::W) {
				key_input.up = true;
				move = true;
			}
			if (sfml_event.key.code == sf::Keyboard::S) {
				key_input.down = true;
				move = true;
			}
			if (sfml_event.key.code == sf::Keyboard::A) {
				key_input.left = true;
				move = true;
			}
			if (sfml_event.key.code == sf::Keyboard::D) {
				key_input.right = true;
				move = true;
			}
			move_flag = move;
		}

		if (!direction_flag) {
			bool changed_dir = false;
			if (sfml_event.key.code == sf::Keyboard::Up) {
				if (players[my_id].direction != DIR_UP) {
					players[my_id].direction = DIR_UP;
					changed_dir = true;
				}
			}
			if (sfml_event.key.code == sf::Keyboard::Down) {
				if (players[my_id].direction != DIR_DOWN) {
					players[my_id].direction = DIR_DOWN;
					changed_dir = true;
				}
			}
			if (sfml_event.key.code == sf::Keyboard::Left) {
				if (players[my_id].direction != DIR_LEFT) {
					players[my_id].direction = DIR_LEFT;
					changed_dir = true;
				}
			}
			if (sfml_event.key.code == sf::Keyboard::Right) {
				if (players[my_id].direction != DIR_RIGHT) {
					players[my_id].direction = DIR_RIGHT;
					changed_dir = true;
				}
			}
			direction_flag = changed_dir;
		}
		
		if (sfml_event.key.code == sf::Keyboard::Space && !attack_flag) {
			attack_flag = true;
		}

		if (sfml_event.key.code == sf::Keyboard::Tab) {
			if (information_mode)
				information_mode = false;
			else {
				information_mode = true;
			}
		}
		if (sfml_event.key.code == sf::Keyboard::T) {
			if (chat_mode)
				chat_mode = false;
			else {
				information_mode = false;
				chat_mode = true;
				chat_start = true;
			}
		}
	}
}

void Game::draw_sfml_text(TI position, char str[], sf::Color color, int size)
{
	sfml_text.setFont(sfml_font);
	sfml_text.setString(str);
	sfml_text.setCharacterSize(size);
	sfml_text.setFillColor(color);
	sfml_text.setPosition(position.x, position.y);
	sfml_window->draw(sfml_text);
}

void Game::draw_sfml_text_s(TI position, string str, sf::Color color, int size)
{
	sfml_text.setFont(sfml_font);
	sfml_text.setString(str);
	sfml_text.setCharacterSize(size);
	sfml_text.setFillColor(color);
	sfml_text.setPosition(position.x, position.y);
	sfml_window->draw(sfml_text);
}

void Game::draw_sfml_rect(TI position, TI size, sf::Color color, sf::Color fill_color)
{
	sf::RectangleShape rect(sf::Vector2f(size.x, size.y));
	rect.setPosition(position.x, position.y);
	rect.setOutlineThickness(1); // Set outline thickness to 1 pixel
	rect.setOutlineColor(color); // Set outline color to white
	rect.setFillColor(fill_color);
	sfml_window->draw(rect);
}
