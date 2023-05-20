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
	
	sfml_window->setFramerateLimit(200);
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

void Game::timer()
{
	unsigned int current_time = static_cast<unsigned>(duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count());
	for (auto& player : players) {
		if (player.second.chat_time != 0) {
			if (player.second.chat_time + 5000 < current_time) {
				player.second.chat_time = 0;
				player.second.chat.clear();
			}
		}
		if (player.second.moved_time + 500 < current_time)
			player.second.state = ST_IDLE;
	
		if (player.second.attack_time + 400 > current_time)
			player.second.state = ST_ATTACK;
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
		if (input_height == 430) {
			input_height = 130;
		}
		else {
			input_height += 100;
		}
		text_input = "";
	}
	else if (sfml_event.type == sf::Event::KeyPressed && sfml_event.key.code == sf::Keyboard::Return && input_height == 430) {
		try_connect = true;
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
	TI relative_position = { (WIDTH / 2) + (position.x - players[my_id].position.x) * BLOCK_SIZE, (HEIGHT / 2) + (position.y - players[my_id].position.y) * BLOCK_SIZE };
	return relative_position;
}

void Game::initialize_ingame()
{
	information_mode = false;
	chat_mode = false;

	scene = 1;
	cout << "You can move with direction keys." << endl;
	cout << "Press Tab to see information." << endl;
}

void Game::draw_game()
{
	//체스 판
	TI player_position = players[my_id].position;
	
	TI chess_board_pixel_size{ BLOCK_SIZE, BLOCK_SIZE };
	TI chess_board_pixel_position;
	for (int i = 0; i < CLIENT_RANGE; i++) {
		for (int j = 0; j < CLIENT_RANGE; j++) {
			chess_board_pixel_position.x = i * BLOCK_SIZE;
			chess_board_pixel_position.y = j * BLOCK_SIZE;
			sf::Color color(100, 50, 0);
			
			if ((player_position.x + i + player_position.y + j) % 2 == 0)
				color = sf::Color(170, 170, 170);
			
			if (player_position.x + i < VIEW_RANGE || player_position.x + i > MAP_SIZE + VIEW_RANGE || player_position.y + j < VIEW_RANGE || player_position.y + j > MAP_SIZE + VIEW_RANGE)
				color = sf::Color(50, 50, 50);

			draw_sfml_rect(chess_board_pixel_position, chess_board_pixel_size, color, color);
		}
	}
	
	// Sort the players by their Y position using qsort and a lambda function
	std::vector<std::pair<int, Player>> sorted_players(players.begin(), players.end());
	
	qsort(sorted_players.data(), sorted_players.size(), sizeof(std::pair<int, Player>), [](const void* a, const void* b) {
		const auto& player1 = *reinterpret_cast<const std::pair<int, Player>*>(a);
		const auto& player2 = *reinterpret_cast<const std::pair<int, Player>*>(b);
		return player1.second.position.y - player2.second.position.y;
	});

	// Draw the players in sorted order
	for (const auto& player : sorted_players) {
		draw_sprite(player.first, sf::Color::White, 4); // Walk
		TI related_pos = get_relative_location(player.second.position);
		string name(player.second.name);
		draw_sfml_text_s({ related_pos.x - (int)name.length() * 4, related_pos.y - 110}, name, sf::Color::White, 14);

		if (player.second.chat_time) {		//draw chat
			draw_sfml_text_s({ related_pos.x - (int)player.second.chat.length() * 4, related_pos.y - 130 }, player.second.chat, sf::Color::White, 14);
		}
	}
	
	if(information_mode)
		draw_information_mode();
	if (chat_mode) 
		draw_chat_mode();
}

void Game::draw_sprite(int id, sf::Color color, char size)
{
	Player* this_player = &players[id];
	
	TI position;
	if (id == my_id)
		position = TI{ WIDTH / 2, HEIGHT / 2 };
	else
		position = get_relative_location(this_player->position);
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
	player_sprite[this_player->state].setTextureRect(sf::IntRect(sprite_i / 20 % sprite_length * sprite_size.x, sprite_size.y * (direction + 1), sprite_size.x, sprite_size.y));
	sfml_window->draw(player_sprite[this_player->state]);
	
	this_player->sprite_iter+=2;

}

void Game::draw_information_mode()
{
	//draw player list
	int text_size = 13;
	draw_sfml_rect(TI{ 10, 10 }, TI{ 130, 730 }, sf::Color(150, 150, 150), sf::Color(150, 150, 150, 200));
	int information_height = 0;
	draw_sfml_text_s(TI{ 15, 10 + information_height++ * (text_size + 2) }, "Ping: " + std::to_string(ping), sf::Color::Black, text_size);
	draw_sfml_text_s(TI{ 15, 10 + information_height++ * (text_size + 2) }, std::to_string(my_id) + ": " +  std::to_string(players[my_id].position.x) + ", " + std::to_string(players[my_id].position.y), sf::Color::Black, text_size);

	for (auto& player : players) {
		if (player.first == my_id)
			continue;
		draw_sfml_text_s(TI{ 15, 10 + information_height++ * (text_size + 2) }, std::to_string(player.first) + ": " + std::to_string(player.second.position.x) + ", " + std::to_string(player.second.position.y), sf::Color::Red, text_size);
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
		TI player_pos_minimap{ player.second.position.x * minimap_size.x / MAP_SIZE + minimap_start_point.x, player.second.position.y * minimap_size.y / MAP_SIZE + minimap_start_point.y };
		if (player.first == my_id)
			continue;
		draw_sfml_rect(player_pos_minimap, TI{ 1, 1 }, sf::Color::Red, sf::Color::Red);	//other position on mini map
	}
	TI player_pos_minimap{ players[my_id].position.x * minimap_size.x / MAP_SIZE + minimap_start_point.x,  players[my_id].position.y * minimap_size.y / MAP_SIZE + minimap_start_point.y};
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
