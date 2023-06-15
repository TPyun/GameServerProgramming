#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include "Game.h"

using namespace std;
using namespace chrono;

Game::Game()
{
	sfml_window = new sf::RenderWindow(sf::VideoMode(WIDTH, HEIGHT), "2D Game");
	if (sfml_window == NULL) {
		cout << "Could not create SFML window!" << endl;
	}
	sfml_window->setFramerateLimit(set_fps);

	if (!sfml_font.loadFromFile("arial.ttf")) {
		cout << "Could not load font!" << endl;
	}
	
	//draw_sfml_text_s(TI{ WIDTH / 2 - players[my_id].size.x / 2 - 90 , HEIGHT / 2 - players[my_id].size.y / 2 }, "Loading Game", sf::Color::White, 30);
	//render();

	//땅 텍스쳐
	if(!grass_texture.loadFromFile("Texture/grass.png"))
		cout << "Could not load grass texture!" << endl;
	grass_texture.setSmooth(true);
	grass_sprite.setTexture(grass_texture);

	//불 텍스쳐
	if(!fire_texture.loadFromFile("Texture/fire.png"))
		cout << "Could not load fire texture!" << endl;
	fire_sprite.setTexture(fire_texture);

	//돌 텍스쳐
	if (!rock_texture.loadFromFile("Texture/Rock.png"))
		cout << "Could not load Rock texture!" << endl;
	rock_sprite.setTexture(rock_texture);
	
	//플레이어 텍스쳐
	char player_tex_file[7][20]{ "Idle_new.png", "Walk_new.png", "Attack.png", "Idle_new.png", "Run_new.png", "Push_new.png", "Hit_new.png" };
	for (int act = 0; act < 7; act++) {
		char player_tex_root[30] = "Texture/Player/";
		if (!player_texture[act].loadFromFile(strcat(player_tex_root, player_tex_file[act])))
			cout << "Image not loaded!" << endl;
		else 
			player_sprite[act].setTexture(player_texture[act]);
	}

	////몬스터 텍스쳐
	//char monster_tex_file[4][20]{ "Idle.png", "Walk.png", "Stomp.png", "Swipe.png" };
	//for (int act = 0; act < 4; act++) {
	//	char monster_tex_root[30] = "Texture/Monster/";
	//	if (!monster_texture[act].loadFromFile(strcat(monster_tex_root, monster_tex_file[act])))
	//		cout << "Image not loaded!" << endl;
	//	else
	//		monster_sprite[act].setTexture(monster_texture[act]);
	//}

	//사운드 로드
	char sound_file[100][20]{"hit.wav", "attack_air.wav", "sword_blood.wav", "sword_air.wav", "walk_grass.wav", "env.wav",  "turn_on.wav", "tab.wav", "dead.wav", "fire.wav", "doom.wav", "stat.wav", "down.wav", "error.wav", "beam.wav"};
	for (int type = 0; type < 15; type++) {
		char sounds_root[30] = "Sounds/";
		if (!sound_buffer[type].loadFromFile(strcat(sounds_root, sound_file[type])))
			cout << "Sound not loaded!" << endl;
	}

	cout << "Press Tab to move another input box" << endl;
	cout << "Press Enter to connect" << endl;
	
	initialize_main();
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
T lerp(const T& a, const T& b, double t) {
	return a + (b - a) * t;
}

void Game::timer()
{
	unsigned int current_time = static_cast<unsigned>(duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count());
	for (auto& player : players) {
		if (current_time - player.second.sprite_time > 100) {
			player.second.sprite_iter++;
			player.second.sprite_time = current_time;
		}
		
		if (player.second.chat_time != 0) {
			if (player.second.chat_time + 5000 < current_time) {
				player.second.chat_time = 0;
				player.second.chat.clear();
			}
		}
		
		bool move_x = true;
		bool move_y = true;
		if (abs(player.second.arr_position.x - player.second.curr_position.x) < 0.05f) {
			player.second.curr_position.x = player.second.arr_position.x;
			player.second.state = ST_IDLE;
			move_x = false;
		}
		if (abs(player.second.arr_position.y - player.second.curr_position.y) < 0.05f) {
			player.second.curr_position.y = player.second.arr_position.y;
			player.second.state = ST_IDLE;
			move_y = false;
		}
		
		//lerp position in same velocity with time
		if(move_x || move_y) {
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
			
			double delay;
			if (player.first < MAX_USER)
				delay = 1000.f / PLAYER_MOVE_TIME;
			else
				delay = 1000.f / NPC_MOVE_TIME;

			double velocity = delay / real_fps;
			player.second.curr_position.x += (double)sign_x * velocity;
			player.second.curr_position.y += (double)sign_y * velocity;
			player.second.state = ST_MOVE;
		}
	
		if (player.second.forward_attack_time + 300 > current_time) {
			player.second.state = ST_FORWARD_ATTACK;
		}
		else if (player.second.wide_attack_time + 500 > current_time) {
			player.second.state = ST_WIDE_ATTACK;
		}
	}

	if (abs(players[my_id].curr_hp - players[my_id].hp) < 1)
		players[my_id].curr_hp = players[my_id].hp;
	else
		players[my_id].curr_hp = lerp(players[my_id].curr_hp, (float)players[my_id].hp, 0.05);

	if (abs(players[my_id].curr_exp - players[my_id].exp) < 1)
		players[my_id].curr_exp = players[my_id].exp;
	else
		players[my_id].curr_exp = lerp(players[my_id].curr_exp, (float)players[my_id].exp, 0.05);

	if (dead_time > 0) {
		left_dead_time = RESPAWN_TIME - (current_time - dead_time);
		if (left_dead_time < 0) {
			dead_time = 0;
			left_dead_time = RESPAWN_TIME;
		}
	}

	if (stat_changed_time > 0) {
		left_stat_chaged_time = STAT_DISPLAY_TIME - (current_time - stat_changed_time);
		if (left_stat_chaged_time < 0) {
			stat_changed_time = 0;
			left_stat_chaged_time = STAT_DISPLAY_TIME;
		}
	}

	if (attack_success_time > 0) {
		left_attack_success_time = STAT_DISPLAY_TIME - (current_time - attack_success_time);
		if (left_attack_success_time < 0) {
			attack_success_time = 0;
			left_attack_success_time = STAT_DISPLAY_TIME;
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
	draw_sfml_rect(TI{ 99, input_height}, TI{200, 20}, sf::Color(80, 80, 80), sf::Color(80, 80, 80), 3);

	draw_sfml_rect(TI{ 99, 130 }, TI{ 200, 20 }, sf::Color::White, sf::Color::Transparent, 3);
	draw_sfml_rect(TI{ 99, 230 }, TI{ 200, 20 }, sf::Color::White, sf::Color::Transparent, 3);
	draw_sfml_rect(TI{ 99, 330 }, TI{ 200, 20 }, sf::Color::White, sf::Color::Transparent, 3);
	draw_sfml_rect(TI{ 99, 430 }, TI{ 200, 20 }, sf::Color::White, sf::Color::Transparent, 3);
	
	draw_sfml_text(TI{ 60, input_height }, (char*)"Tab", sf::Color(200, 200, 200), 17);
	
	draw_sfml_text(TI{ 100, 100 }, (char*)"IP Address", sf::Color(200, 200, 200), 17);
	draw_sfml_text(TI{ 100, 200 }, (char*)"Port", sf::Color(200, 200, 200), 17);
	draw_sfml_text(TI{ 100, 300 }, (char*)"Name", sf::Color(200, 200, 200), 17);
	draw_sfml_text(TI{ 100, 400 }, (char*)"Play Game", sf::Color(200, 200, 200), 17);
	
	draw_sfml_text(TI{ 100, 130 }, (char*)ip_address, sf::Color(200, 200, 200), 17);
	draw_sfml_text(TI{ 100, 230 }, (char*)Port, sf::Color(200, 200, 200), 17);
	draw_sfml_text(TI{ 100, 330 }, (char*)Name, sf::Color(200, 200, 200), 17);
	draw_sfml_text(TI{ 100, 430 }, (char*)"Press Enter", sf::Color(200, 200, 200), 17);
	

	if (input_warning) {
		draw_sfml_text(TI{ 100, 550 }, (char*)"Please Fill All Fields", sf::Color(200, 0, 0), 17);
	}
	if (connect_warning) {
		draw_sfml_text(TI{ 100, 500 }, (char*)"Connection Fail", sf::Color(180, 0, 0), 17);
	}
	if (id_warning) {
		draw_sfml_text(TI{ 100, 500 }, (char*)"ID is already exist", sf::Color(180, 0, 0), 17);
	}
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
		play_sound(SOUND_TAB, false, 100);
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
		else {
			play_sound(SOUND_ERROR, false, 100);
			input_warning = true;
		}
	}
	else if (sfml_event.type == sf::Event::TextEntered && text_input.size() < 15) {
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

TI Game::get_relative_location(TD position)
{
	TI relative_position = { (WIDTH / 2) + (position.x - players[my_id].curr_position.x) * BLOCK_SIZE, (HEIGHT / 2) + (position.y - players[my_id].curr_position.y) * BLOCK_SIZE };
	return relative_position;
}

void Game::initialize_main()
{
	stop_sound(SOUND_ENV);
	play_sound(SOUND_BEAM, false, 100);
	ZeroMemory(&key_input, sizeof(key_input));

	input_height = 130;
	text_input = ip_address;
	input_warning = false;
	scene = 0;
}

void Game::initialize_ingame()
{
	play_sound(SOUND_ENV, true, 10);
	play_sound(SOUND_BEAM, false, 100);
	ZeroMemory(&key_input, sizeof(key_input));

	information_mode = false;
	chat_mode = false;
	scene = 1;
	
	cout << "You can move with direction keys." << endl;
	cout << "Press Tab to see information." << endl;
}

void Game::play_sound(char type, bool loop, int volume)
{
	/*int i = 0;
	for (auto& sound : sounds) {
		cout << i << " " << sound.getStatus() << endl;
		i++;
	}*/
	for (auto& sound : sounds) {
		if (sound.getStatus() == 0) {
			sound.setBuffer(sound_buffer[type]);
			sound.setLoop(loop);
			sound.setVolume(volume);
			sound.play();
			return;
		}
	}
}

void Game::stop_sound(char type)
{
	for (auto& sound : sounds) {
		if (sound.getStatus() != 0 && sound.getBuffer() == &sound_buffer[type]) {
			sound.stop();
		}
	}
}

void Game::draw_game()
{
	TD player_position = players[my_id].curr_position;
	
	// 소수점
	TF integer_part;
	TI integer_real;
	TF fraction;
	fraction.x = modf(player_position.x, &integer_part.x);
	fraction.y = modf(player_position.y, &integer_part.y);
	integer_real.x = (int)integer_part.x;
	integer_real.y = (int)integer_part.y;

	TF offset_pos;
	offset_pos.x = integer_real.x % CLIENT_RANGE + fraction.x;
	offset_pos.y = integer_real.y % CLIENT_RANGE + fraction.y;

	// Draw sand texture
	double sprite_scale_x = (double)WIDTH / (double)grass_texture.getSize().x;
	double sprite_scale_y = (double)HEIGHT / (double)grass_texture.getSize().y;
	grass_sprite.setScale(sprite_scale_x, sprite_scale_y);
	for (int x = 0; x <= 1; x++) {
		for (int y = 0; y <= 1; y++) {
			int sprite_pos_x = WIDTH * x - offset_pos.x * WIDTH / CLIENT_RANGE;
			int sprite_pos_y = HEIGHT * y - offset_pos.y * HEIGHT / CLIENT_RANGE;
			grass_sprite.setPosition(sprite_pos_x, sprite_pos_y);
			sfml_window->draw(grass_sprite);
		}
	}
	
	// Draw the players in sorted order
	for (int height = - VIEW_RANGE; height <= VIEW_RANGE; height++) {
		for (const auto& player : players) {
			if ((int)player_position.y + height != player.second.arr_position.y) continue;

			sf::Color player_color = sf::Color::White;
			if (player.first >= OBSTACLE_START) {
				draw_obstacle(player.first);
				continue;
			}
			else if (player.first >= AGGR_NPC_START)	//NPC
				player_color = sf::Color(255, 150, 150, 200);
			else if(player.first >= NORMAL_NPC_START)	//NPC
				player_color = sf::Color(200,200,200,200);
			else if(player.first == my_id)				//본인
				player_color = sf::Color::White;
			else										//다른 플레이어
				player_color = sf::Color(150, 255, 150, 200);
			draw_player_sprite(player.first, player_color, 3);
			TI related_pos = get_relative_location(player.second.curr_position);
			string name(player.second.name);
			draw_sfml_text_s({ related_pos.x - (int)name.length() * 4, related_pos.y - 80 }, name, sf::Color::White, 14);
			if (player.second.chat_time)		//draw chat
				draw_sfml_text_s({ related_pos.x - (int)player.second.chat.length() * 4, related_pos.y - 100 }, player.second.chat, sf::Color::White, 14);
		}
	}

	//draw distance rect
	if (distance_debug_mode) {
		for (int x = 0; x < CLIENT_RANGE; x++) {
			for (int y = 0; y < CLIENT_RANGE; y++) {
				draw_sfml_rect({ BLOCK_SIZE * x,  BLOCK_SIZE * y }, { BLOCK_SIZE, BLOCK_SIZE }, sf::Color::Red, sf::Color::Transparent, 3);
			}
		}
		draw_sfml_rect({ WIDTH / 2 - int(BLOCK_SIZE * SECTOR_SIZE / 2), HEIGHT / 2 - int(BLOCK_SIZE * SECTOR_SIZE / 2) }, { BLOCK_SIZE * SECTOR_SIZE, BLOCK_SIZE * SECTOR_SIZE }, sf::Color::Blue, sf::Color::Transparent, 3);
	}
	
	draw_stat();
	if(information_mode)
		draw_information_mode();
	if (chat_mode) 
		draw_chat_mode();
	if (dead) {
		draw_sfml_rect({ 0 , 0 }, { WIDTH, HEIGHT }, sf::Color(0, 0, 0, 200), sf::Color(0, 0, 0, 200), 3);
		string dead_sign = "YOU ARE DEAD";
		draw_sfml_text_s({ WIDTH / 2 - (int)dead_sign.length() * 9 , HEIGHT / 2 - 50}, dead_sign, sf::Color::Red, 30);

		string left_time = to_string((int)left_dead_time / 1000 + 1);
		draw_sfml_text_s({ WIDTH / 2 - (int)left_time.length() * 4 , HEIGHT / 2 + 50 }, left_time, sf::Color::White, 30);
	}
}

void Game::draw_obstacle(int id)
{
	Player* this_obstacle = &players[id];
	TI position = get_relative_location(this_obstacle->curr_position);
	TI size;
	size.x = rock_texture.getSize().x;
	size.y = rock_texture.getSize().y;

	rock_sprite.setScale(0.3, 0.3);
	rock_sprite.setPosition(position.x - (size.x / 2) * 0.3, position.y - (size.y / 2) * 0.3 - 20);
	sfml_window->draw(rock_sprite);
}

void Game::draw_stat()
{
	//draw hp, level, exp gauge
	draw_sfml_text_s({ 10, 10 }, "HP", sf::Color::White, 15);
	draw_sfml_text_s({ 10, 25 }, "EXP", sf::Color::White, 15);
	draw_sfml_text_s({ 10, 40 }, "Level: " + to_string(players[my_id].level), sf::Color::White, 15);

	int hp = players[my_id].curr_hp;
	int max_hp = players[my_id].max_hp;
	int level = players[my_id].level;
	int exp = players[my_id].curr_exp;
	
	int hp_gauge = (int)(200.f * (float)hp / max_hp);
	int exp_gauge = (200.f * (float)exp / (100.f * pow(2, level - 1)));
	
	draw_sfml_rect({ 50, 10 }, { hp_gauge, 15 }, sf::Color::Transparent, sf::Color::Red, 3);
	draw_sfml_rect({ 50, 25 }, { exp_gauge, 15 }, sf::Color::Transparent, sf::Color::Green, 3);
	
	draw_sfml_rect({ 50, 10 }, { 200, 30 }, sf::Color::White, sf::Color::Transparent, 3);

	if (stat_changed_time > 0) {
		if (level_change != 0) {
			string text = "Level UP " + to_string(level_change);
			draw_sfml_text_s({ WIDTH / 2 - (int)text.length() * 6  , HEIGHT / 2 - 130 }, text, sf::Color::White, 22);
		}
		else if (exp_change > 0) {
			string text = "EXP UP " + to_string(exp_change);
			draw_sfml_text_s({ WIDTH / 2 - (int)text.length() * 6  , HEIGHT / 2 - 130 }, text, sf::Color::White, 22);
		}
		else if (hp_change < 0) {
			string text = "HP " + to_string(hp_change);
			draw_sfml_text_s({ WIDTH / 2 - (int)text.length() * 6  , HEIGHT / 2 - 130 }, text, sf::Color::Red, 22);
		}
		else if (hp_change > 0) {
			string text = "HP " + to_string(hp_change);
			draw_sfml_text_s({ WIDTH / 2 - (int)text.length() * 6  , HEIGHT / 2 - 130 }, text, sf::Color::Green, 22);
		}
	}

	if (attack_success_time > 0) {
		if (ATTACK_FORWARD == attack_success_type) {
			string text = "ATTACK 50";
			draw_sfml_text_s({ WIDTH / 2 - (int)text.length() * 6, HEIGHT / 2 }, text, sf::Color::White, 22);
		}
		else if (ATTACK_WIDE) {
			string text = "ATTACK 20";
			draw_sfml_text_s({ WIDTH / 2 - (int)text.length() * 6, HEIGHT / 2 }, text, sf::Color::White, 22);
		}
	}
}

void Game::draw_player_sprite(int id, sf::Color color, char size)
{
	Player* this_player = &players[id];
	
	TI player_position;
	if (id == my_id)
		player_position = TI{ WIDTH / 2, HEIGHT / 2 };
	else
		player_position = get_relative_location(this_player->curr_position);
	
	TUC player_sprite_size{};
	char player_sprite_length = 4;
	char direction = this_player->direction;
	unsigned char sprite_i = this_player->sprite_iter;

	switch (this_player->state) {
	case ST_MOVE:
	case ST_IDLE:
		player_sprite_size = { 16, 24 };
		break;
	case ST_FORWARD_ATTACK:
		player_sprite_size = { 24, 24 };
		break;
	case ST_WIDE_ATTACK:
		player_sprite_size = { 16, 24 };
		
		int fire_sprite_length = 5;
		int fire_sprite_size = 16;
		fire_sprite.setScale(size, size);
		fire_sprite.setTextureRect(sf::IntRect(sprite_i % fire_sprite_length * fire_sprite_size, 0, fire_sprite_size, fire_sprite_size));
		for (int x = -1; x <= 1; x++) {
			for (int y = -1; y <= 1; y++) {
				if (x * y != 0) continue;
				fire_sprite.setPosition(player_position.x + BLOCK_SIZE * x - (fire_sprite_size * size / 2), player_position.y + BLOCK_SIZE * y - (fire_sprite_size * size));
				sfml_window->draw(fire_sprite);
			}
		}
		break;
	}
	
	player_sprite[this_player->state].setColor(color);
	player_sprite[this_player->state].setScale(size, size * 1.1f);
	player_sprite[this_player->state].setPosition(player_position.x - (player_sprite_size.x * size) / 2, player_position.y - (player_sprite_size.y * size));
	player_sprite[this_player->state].setTextureRect(sf::IntRect(sprite_i % player_sprite_length * player_sprite_size.x, player_sprite_size.y * (direction + 1), player_sprite_size.x, player_sprite_size.y));
	sfml_window->draw(player_sprite[this_player->state]);
}

void Game::draw_monster_sprite(int id, sf::Color color, char size)
{
	Player* this_monster = &players[id];
	TI monster_position = get_relative_location(this_monster->curr_position);
	char monster_sprite_length{};
	TUC monster_sprite_size{};
	char direction{};
	unsigned char sprite_i = this_monster->sprite_iter;
	monster_sprite_size = { 64, 64 };

	switch (this_monster->state) {
	case ST_MOVE:
		monster_sprite_length = 10;
		switch (this_monster->direction) 
		{
		case DIR_UP:
			direction = 0;
			break;
		case DIR_DOWN:
			direction = 3;
			break;
		case DIR_LEFT:
			direction = 1;
			break;
		case DIR_RIGHT:
			direction = 2;
			break;
		}
		break;
	case ST_IDLE:
		monster_sprite_length = 7;
		direction = this_monster->direction;
		switch (this_monster->direction) 
		{
		case DIR_UP:
			direction = 1;
			break;
		case DIR_DOWN:
			direction = 0;
			break;
		case DIR_LEFT:
			direction = 2;
			break;
		case DIR_RIGHT:
			direction = 3;
			break;
		}
		break;
	case ST_FORWARD_ATTACK:
		monster_sprite_length = 13;
		direction = this_monster->direction;
		switch (this_monster->direction) {
		case DIR_UP:
			direction = 0;
			break;
		case DIR_DOWN:
			direction = 3;
			break;
		case DIR_LEFT:
			direction = 1;
			break;
		case DIR_RIGHT:
			direction = 2;
			break;
		}
		break;
	case ST_WIDE_ATTACK:
		monster_sprite_length = 10;
		direction = this_monster->direction;
		switch (this_monster->direction) {
		case DIR_UP:
			direction = 1;
			break;
		case DIR_DOWN:
			direction = 0;
			break;
		case DIR_LEFT:
			direction = 2;
			break;
		case DIR_RIGHT:
			direction = 3;
			break;
		}
		int fire_sprite_length = 5;
		int fire_sprite_size = 16;
		fire_sprite.setScale(size, size);
		fire_sprite.setTextureRect(sf::IntRect(sprite_i % fire_sprite_length * fire_sprite_size, 0, fire_sprite_size, fire_sprite_size));
		for (int x = -1; x <= 1; x++) {
			for (int y = -1; y <= 1; y++) {
				if (x * y != 0) continue;
				fire_sprite.setPosition(monster_position.x + BLOCK_SIZE * x - (fire_sprite_size * size / 2), monster_position.y + BLOCK_SIZE * y - (fire_sprite_size * size));
				sfml_window->draw(fire_sprite);
			}
		}
		break;
	}
	monster_sprite[this_monster->state].setColor(color);
	monster_sprite[this_monster->state].setScale(size, size);
	monster_sprite[this_monster->state].setPosition(monster_position.x - (monster_sprite_size.x * size) / 2, monster_position.y - (monster_sprite_size.y * size));
	monster_sprite[this_monster->state].setTextureRect(sf::IntRect(sprite_i % monster_sprite_length * monster_sprite_size.x, monster_sprite_size.y * direction, monster_sprite_size.x, monster_sprite_size.y));
	sfml_window->draw(monster_sprite[this_monster->state]);
}

void Game::draw_information_mode()
{
	//draw player list
	int text_size = 13;
	draw_sfml_rect(TI{ 10, 10 }, TI{ 130, WIDTH - 20 }, sf::Color(150, 150, 150), sf::Color(150, 150, 150, 200), 1);
	int information_height = 0;
	draw_sfml_text_s(TI{ 15, 10 + information_height++ * (text_size + 2) }, "Ping: " + std::to_string(ping), sf::Color::Black, text_size);
	draw_sfml_text_s(TI{ 15, 10 + information_height++ * (text_size + 2) }, std::to_string(my_id) + ": " +  std::to_string(players[my_id].arr_position.x) + ", " + std::to_string(players[my_id].arr_position.y), sf::Color::Black, text_size);

	for (auto& player : players) {
		if (player.first == my_id)
			continue;
		sf::Color color = sf::Color::White;
		if (player.first >= OBSTACLE_START)
			color = sf::Color::Black;
		else if (player.first >= AGGR_NPC_START) 		//NPC
			color = sf::Color::Red;
		else if (player.first >= NORMAL_NPC_START)	//NPC
			color = sf::Color::Red;
		else if (player.first == my_id)				//본인
			color = sf::Color::White;
		else											//다른 플레이어
			color = sf::Color::Green;
		draw_sfml_text_s(TI{ 15, 10 + information_height++ * (text_size + 2) }, std::to_string(player.first) + ": " + std::to_string(player.second.arr_position.x) + ", " + std::to_string(player.second.arr_position.y), color, text_size);
	}

	//draw map
	int minimap_frame_size = 590;
	TI minimap_frame_start_point{ 150, 10 };
	int minimap_offset = 50;
	TI minimap_start_point{ minimap_frame_start_point.x + minimap_offset / 2, minimap_frame_start_point.y + minimap_offset  / 2};
	TI minimap_size{ minimap_frame_size - minimap_offset, minimap_frame_size - minimap_offset };
	draw_sfml_rect(minimap_frame_start_point, TI{ minimap_frame_size, minimap_frame_size }, sf::Color(150, 150, 150), sf::Color(150, 150, 150, 200), 1);	//mini map frame
	draw_sfml_rect(minimap_start_point, minimap_size, sf::Color::Black, sf::Color::Black, 1);	//mini map
	
	for (auto& player : players) {
		TI player_pos_minimap{ player.second.curr_position.x * minimap_size.x / W_WIDTH + minimap_start_point.x, player.second.curr_position.y * minimap_size.y / W_HEIGHT + minimap_start_point.y };
		if (player.first == my_id)
			continue;
		draw_sfml_rect(player_pos_minimap, TI{ 1, 1 }, sf::Color::Red, sf::Color::Red, 1);	//other position on mini map

	}
	TI player_pos_minimap{ players[my_id].curr_position.x * minimap_size.x / W_WIDTH + minimap_start_point.x,  players[my_id].curr_position.y * minimap_size.y / W_HEIGHT + minimap_start_point.y};
	draw_sfml_rect(player_pos_minimap, TI{ 1, 1 }, sf::Color::White, sf::Color::White, 1);	//my position on mini map
}

void Game::draw_chat_mode()
{
	draw_sfml_rect(TI{ 10, HEIGHT - 30 }, TI{ 400, 20 }, sf::Color(150, 150, 150), sf::Color(150, 150, 150, 200), 1);
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
	if (sfml_event.type == sf::Event::KeyReleased) {
		if (sfml_event.key.code == sf::Keyboard::W) {
			key_input.up = false;
		}
		if (sfml_event.key.code == sf::Keyboard::S) {
			key_input.down = false;
		}
		if (sfml_event.key.code == sf::Keyboard::A) {
			key_input.left = false;
		}
		if (sfml_event.key.code == sf::Keyboard::D) {
			key_input.right = false;
		}
	}
	
	if (sfml_event.type == sf::Event::KeyPressed) {
		if (sfml_event.key.code == sf::Keyboard::W) {
			key_input.up = true;
		}
		if (sfml_event.key.code == sf::Keyboard::S) {
			key_input.down = true;
		}
		if (sfml_event.key.code == sf::Keyboard::A) {
			key_input.left = true;
		}
		if (sfml_event.key.code == sf::Keyboard::D) {
			key_input.right = true;
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
		
		if (sfml_event.key.code == sf::Keyboard::Space && !forward_attack_flag) {
			forward_attack_flag = true;
		}

		if (sfml_event.key.code == sf::Keyboard::Q && !wide_attack_flag) {
			wide_attack_flag = true;
		}

		if (sfml_event.key.code == sf::Keyboard::Tab) {
			play_sound(SOUND_TAB, false, 100);
			if (information_mode)
				information_mode = false;
			else
				information_mode = true;
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
		
		if (sfml_event.key.code == sf::Keyboard::B) {
			if (distance_debug_mode)
				distance_debug_mode = false;
			else
				distance_debug_mode = true;
		}
	}
	if (key_input.up || key_input.down || key_input.left || key_input.right) {
		move_flag = true;
	}
	else
		move_flag = false;
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

void Game::draw_sfml_rect(TI position, TI size, sf::Color color, sf::Color fill_color, int thickness)
{
	sf::RectangleShape rect(sf::Vector2f(size.x, size.y));
	rect.setPosition(position.x, position.y);
	rect.setOutlineThickness(thickness); // Set outline thickness to 1 pixel
	rect.setOutlineColor(color); // Set outline color to white
	rect.setFillColor(fill_color);
	sfml_window->draw(rect);
}
