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
	cout << "Game initialized!" << endl;
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
				std::cout << "Game closed!" << std::endl;
				isRunning = false;
				return;
			}
			else if (scene == 1) {
				cout << "Disconnected!" << endl;
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
	
	draw_sfml_text(TI{ 60, input_height }, (char*)"Tab", sf::Color(200, 200, 200));
	
	draw_sfml_text(TI{ 100, 100 }, (char*)"IP Address", sf::Color(200, 200, 200));
	draw_sfml_text(TI{ 100, 200 }, (char*)"Port", sf::Color(200, 200, 200));
	draw_sfml_text(TI{ 100, 300 }, (char*)"Play Game", sf::Color(200, 200, 200));
	
	draw_sfml_text(TI{ 100, 130 }, (char*)ip_address, sf::Color(200, 200, 200));
	draw_sfml_text(TI{ 100, 230 }, (char*)Port, sf::Color(200, 200, 200));
	draw_sfml_text(TI{ 100, 330 }, (char*)"Press Enter", sf::Color(200, 200, 200));
}

void Game::main_handle_events()
{
	if (sfml_event.type == sf::Event::KeyPressed && sfml_event.key.code == sf::Keyboard::Backspace && text_input.size()) {
		cout << "Back" << endl;
		text_input.pop_back();
		if (input_height == 130) {
			strcpy(ip_address, text_input.c_str());
		}
		else if (input_height == 230) {
			strcpy(Port, text_input.c_str());
		}
	}
	else if (sfml_event.type == sf::Event::KeyPressed && sfml_event.key.code == sf::Keyboard::Tab) {
		cout << "Tab" << endl;
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
		cout << "Typing: " << static_cast<char>(sfml_event.text.unicode) << endl;
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

TI Game::sfml_get_corrected_position(TI position, TI size)
{
	return TI{ position.x - size.x / 2, position.y - size.y / 2 };
}

void Game::draw_game()
{
	//체스 판
	TI chess_board_pixel_size{ 100, 100 };
	TI chess_board_pixel_position;
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			chess_board_pixel_position.x = i * 100;
			chess_board_pixel_position.y = j * 100;
			sf::Color color(100, 50, 0);
			if ((i + j) % 2 == 0) {
				color = sf::Color(200, 200, 200);
			}
			draw_sfml_rect(chess_board_pixel_position, chess_board_pixel_size, color, color);
		}
	}

	//draw player
	sf::Color color;
	mtx.lock();
	for (auto& player : players) {
		if (player.first == 0)
			continue;
		color = sf::Color(200, 0, 0);
		draw_sfml_rect(sfml_get_corrected_position(player.second.position, player.second.size), player.second.size, color, color);
	}
	mtx.unlock();
	color = sf::Color(0, 0, 0);
	draw_sfml_rect(sfml_get_corrected_position(players[0].position, players[0].size), players[0].size, color, color);
}

void Game::game_handle_events()
{
	if (sfml_event.type == sf::Event::KeyPressed) {
		if (sfml_event.key.code == sf::Keyboard::Up) {
			//std::cout << "UP" << std::endl;
			key_input.up = true;
		}
		if (sfml_event.key.code == sf::Keyboard::Down) {
			//std::cout << "DOWN" << std::endl;
			key_input.down = true;
		}
		if (sfml_event.key.code == sf::Keyboard::Left) {
			//std::cout << "LEFT" << std::endl;
			key_input.left = true;
		}
		if (sfml_event.key.code == sf::Keyboard::Right) {
			//std::cout << "RIGHT" << std::endl;
			key_input.right = true;
		}
	}
}

void Game::draw_sfml_text(TI position, char str[], sf::Color color)
{
	sfml_text.setFont(sfml_font);
	sfml_text.setString(str);
	sfml_text.setCharacterSize(17);
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
	rect.setFillColor(fill_color); // Set fill color to transparent
	sfml_window->draw(rect);
}

