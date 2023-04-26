#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include "Game.h"

using namespace std;
Game::Game()
{
	sfml_window = new sf::RenderWindow(sf::VideoMode(window_size.x, window_size.y), "SFML window");
	if (sfml_window == NULL) {
		cout << "Could not create SFML window!" << endl;
	}
	if (!sfml_font.loadFromFile("arial.ttf")) {
		cout << "Could not load font!" << endl;
	}
	//cout << "Game initialized!" << endl;
	
	sfml_window->setFramerateLimit(60);
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
			else if (scene == 1) {
				//cout << "Disconnected!" << endl;
				connected = false;
				return;
			}
		}
	}

	if (scene == 0) {
		draw_main();
		if (input)
			main_handle_events();
	}
	else if (scene == 1) {
		draw_game();
		if (input)
			game_handle_events();
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
	
	draw_sfml_text(TI{ 60, input_height }, (char*)"Tab", sf::Color(200, 200, 200), 17);
	
	draw_sfml_text(TI{ 100, 100 }, (char*)"IP Address", sf::Color(200, 200, 200), 17);
	draw_sfml_text(TI{ 100, 200 }, (char*)"Port", sf::Color(200, 200, 200), 17);
	draw_sfml_text(TI{ 100, 300 }, (char*)"Play Game", sf::Color(200, 200, 200), 17);
	
	draw_sfml_text(TI{ 100, 130 }, (char*)ip_address, sf::Color(200, 200, 200), 17);
	draw_sfml_text(TI{ 100, 230 }, (char*)Port, sf::Color(200, 200, 200), 17);
	draw_sfml_text(TI{ 100, 330 }, (char*)"Press Enter", sf::Color(200, 200, 200), 17);
}

void Game::main_handle_events()
{
	if (sfml_event.type == sf::Event::KeyPressed && sfml_event.key.code == sf::Keyboard::Backspace && text_input.size()) {
		//cout << "Back" << endl;
		text_input.pop_back();
		if (input_height == 130) {
			strcpy(ip_address, text_input.c_str());
		}
		else if (input_height == 230) {
			strcpy(Port, text_input.c_str());
		}
	}
	else if (sfml_event.type == sf::Event::KeyPressed && sfml_event.key.code == sf::Keyboard::Tab) {
		//cout << "Tab" << endl;
		if (input_height == 330) {
			input_height = 130;
		}
		else {
			input_height += 100;
		}
		text_input = "";
	}
	else if (sfml_event.type == sf::Event::KeyPressed && sfml_event.key.code == sf::Keyboard::Return && input_height == 330) {
		try_connect = true;
	}
	else if (sfml_event.type == sf::Event::TextEntered && text_input.size() < 20) {
		//cout << "Typing: " << static_cast<char>(sfml_event.text.unicode) << endl;
		// Only add ASCII characters
		if (static_cast<char>(sfml_event.text.unicode) > 30) {		//지우는거랑 엔터 안먹히게 제한
			text_input += static_cast<char>(sfml_event.text.unicode);
			if (input_height == 130) {
				strcpy(ip_address, text_input.c_str());
			}
			else if (input_height == 230) {
				strcpy(Port, text_input.c_str());
			}
		}
	}
}

TI Game::get_relative_location(TI position, TI size)
{
	return TI{ (WIDTH / 2) - (size.x / 2) + (position.x - players[my_id].position.x) * BLOCK_SIZE, (HEIGHT / 2) - (size.y / 2) + (position.y - players[my_id].position.y) * BLOCK_SIZE };
}

void Game::initialize_ingame()
{
	information_mode = false;
	scene = 1;
	cout << "You can move with direction keys." << endl;
	cout << "Press Tab to see information." << endl;
}

void Game::draw_game()
{
	//체스 판
	TI chess_board_pixel_size{ BLOCK_SIZE, BLOCK_SIZE };
	TI chess_board_pixel_position;
	for (int i = 0; i < CLIENT_RANGE; i++) {
		for (int j = 0; j < CLIENT_RANGE; j++) {
			chess_board_pixel_position.x = i * BLOCK_SIZE +1;
			chess_board_pixel_position.y = j * BLOCK_SIZE +1;
			sf::Color color(100, 50, 0);
			if ((players[my_id].position.x + i + players[my_id].position.y + j) % 2 == 0) {
				color = sf::Color(170, 170, 170);
			}
			if ( players[my_id].position.x + i < VIEW_RANGE || players[my_id].position.x + i > MAP_SIZE + VIEW_RANGE || players[my_id].position.y + j < VIEW_RANGE || players[my_id].position.y + j > MAP_SIZE + VIEW_RANGE)
				color = sf::Color(50, 50, 50); 
			draw_sfml_rect(chess_board_pixel_position, chess_board_pixel_size, color, color);
		}
	}

	//draw player
	sf::Color color;
	mtx.lock();
	for (auto& player : players) {
		if (player.first == my_id)
			continue;
		color = sf::Color(200, 0, 0);
		draw_sfml_rect(get_relative_location(player.second.position, player.second.size), player.second.size, color, color);								//draw client
		draw_sfml_text_s(get_relative_location(player.second.position, player.second.size), std::to_string(player.first), sf::Color(255, 255, 255), 9);	//draw id of clients
	}
	mtx.unlock();
	
	color = sf::Color(0, 0, 0);
	draw_sfml_rect(TI{ WIDTH / 2 - players[my_id].size.x / 2 , HEIGHT / 2  - players[my_id].size.y / 2 }, players[my_id].size, color, color);						//draw me
	draw_sfml_text_s(TI{ WIDTH / 2 - players[my_id].size.x / 2, HEIGHT / 2 - players[my_id].size.y / 2 }, std::to_string(my_id), sf::Color(255, 255, 255), 9);	//draw number of me

	if(information_mode)
		draw_information();
}

void Game::draw_information()
{
	//draw player list
	draw_sfml_rect(TI{ 10, 10 }, TI{ 130, 730 }, sf::Color(150, 150, 150), sf::Color(150, 150, 150, 200));
	int information_height = 0;
	draw_sfml_text_s(TI{ 15, 10 + information_height++ * 20 },"Ping: "+ std::to_string(ping), sf::Color::Black, 17);
	draw_sfml_text_s(TI{ 15, 10 + information_height++ * 20 }, std::to_string(my_id) + ": " +  std::to_string(players[my_id].position.x) + ", " + std::to_string(players[my_id].position.y), sf::Color::Black, 17);
	for (auto& player : players) {
		if (player.first == my_id)
			continue;
		draw_sfml_text_s(TI{ 15, 10 + information_height++ * 20 }, std::to_string(player.first) + ": " + std::to_string(player.second.position.x) + ", " + std::to_string(player.second.position.y), sf::Color::Red, 17);
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

void Game::game_handle_events()
{
	if (sfml_event.type == sf::Event::KeyPressed) {
		if (!move_flag) {
			bool move = false;
			ZeroMemory(&key_input, sizeof(KS));
			if (sfml_event.key.code == sf::Keyboard::Up) {
				key_input.up = true;
				move = true;
			}
			if (sfml_event.key.code == sf::Keyboard::Down) {
				key_input.down = true;
				move = true;
			}
			if (sfml_event.key.code == sf::Keyboard::Left) {
				key_input.left = true;
				move = true;
			}
			if (sfml_event.key.code == sf::Keyboard::Right) {
				key_input.right = true;
				move = true;
			}
			move_flag = move;
		}

		if (sfml_event.key.code == sf::Keyboard::Tab) {
			if (information_mode)
				information_mode = false;
			else
				information_mode = true;
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

