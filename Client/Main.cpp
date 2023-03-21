/// ======================
///	WARNING!!!!!!!!!!!!!!!
///	YOU HAVE TO RUN AS X64
/// ======================
#include "Game.h"
#include "Network.h"
#pragma comment(lib, "WS2_32.LIB")
using namespace std;
int SDL_main(int argc, char* argv[])
{
	Game* game = new Game();
	HANDLE h_thread = CreateThread(NULL, 0, process, game, 0, NULL);

	
	while (game->get_running()) {
		game->clear();
		game->update();
		game->render();
	}
	
	return 0;
}

//#include <sstream>
//#include "Game.h"
//#pragma comment(lib, "WS2_32.LIB")
//using namespace std;
//
//constexpr short SERVER_PORT = 9000;
//constexpr int BUF_SIZE = 4096;
//
//Game game;
//SOCKET server_socket;
//WSAOVERLAPPED over;
//WSABUF wsa_buffer;
////char buffer[BUF_SIZE];
//
//void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags);
//
//void do_send_message()
//{
//	//���ۿ� ���� �־��ֱ�
//	//cout << "Enter Messsage: ";
//	//cin.getline(buffer, BUF_SIZE - 1);	//���ۿ� �޽��� �Է¹ޱ�
//	//wsa_buffer.buf = buffer;	//������ �ּ�
//	//wsa_buffer.len = static_cast<int>(strlen(buffer)) + 1;		//+1�� �ι��ڱ��� �����ϱ� ����
//
//	wsa_buffer.buf = (char*)&game.key_input;
//	wsa_buffer.len = sizeof(game.key_input);
//	
//	memset(&over, 0, sizeof(over));
//	WSASend(server_socket, &wsa_buffer, 1, 0, 0, &over, send_callback);
//}
//
//void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED recv_over, DWORD flags)
//{
//	//char* packet = wsa_buffer.buf;
//	//
//	//while (packet < wsa_buffer.buf + num_bytes) {
//	//	//��Ŷ ���
//	//	char packet_size = *packet;	//������ ù��° ����Ʈ�� ��Ŷ�� ũ��
//	//	int client_id = *(packet + 1);	//������ �ι�° ����Ʈ�� Ŭ���̾�Ʈ ���̵�
//	//	//cout << "Client[" << client_id << "] Sent[" << packet_size - 2 << "bytes] : " << packet + 2 << endl;	//������ ����° ����Ʈ���ʹ� ���� �޽���
//	//	memcpy(&game.key_input, packet + 2, sizeof(game.key_input));
//	//	packet = packet + packet_size;	//���� ��Ŷ���� �̵�
//	//}
//	
//	do_send_message();
//}
//
//void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags)
//{
//	game.key_input = { false, false, false, false };
//	
//	wsa_buffer.buf = (char*)&game.player->position;
//	wsa_buffer.len = sizeof(game.player->position);
//	
//	DWORD recv_flag = 0;
//	memset(over, 0, sizeof(*over));
//	WSARecv(server_socket, &wsa_buffer, 1, 0, &recv_flag, over, recv_callback);
//}
//
//int SDL_main(int argc, char* argv[])
//{
//	while (!game.try_connect) {
//		Sleep(100);
//	}
//	
//	WSADATA WSAData;
//	WSAStartup(MAKEWORD(2, 0), &WSAData);
//	server_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, 0);
//	SOCKADDR_IN server_address;
//	ZeroMemory(&server_address, sizeof(server_address));
//	server_address.sin_family = AF_INET;
//
//	string str(game.Port);
//	int port_num = 0;
//	stringstream ssInt(str);
//	ssInt >> port_num;
//
//	server_address.sin_port = htons(port_num);
//	inet_pton(AF_INET, game.IPAdress, &server_address.sin_addr);
//	
//	int retval = WSAConnect(server_socket, reinterpret_cast<sockaddr*>(&server_address), sizeof(server_address), 0, 0, 0, 0);
//	if (retval == SOCKET_ERROR) {
//		cout << "Connection error" << endl;
//		return 0;
//	}
//	cout << "Connected to server" << endl;
//	
//	
//	do_send_message();
//	game.scene = 1;
//	
//	while (game.get_running()) {
//		game.clear();
//		game.update();
//		game.render();
//		SleepEx(10, true);	
//	}
//	
//	closesocket(server_socket);
//	WSACleanup();
//	return 0;
//}
