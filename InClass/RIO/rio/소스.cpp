#include <iostream>
#include <map>
#include <queue>
using namespace std;
#include <WS2tcpip.h>
#include <MSWSock.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

#define MAX_BUFFER      1024
#define SERVER_PORT     3500
#define MAX_CLIENTS		10

#define SESSION_BUFFER_SIZE 65536
#define MAX_RIO_RESULTS	256
#define MAX_SEND_RQ_SIZE_PER_SOCKET  32
#define MAX_RECV_RQ_SIZE_PER_SOCKET  32
#define MAX_CQ_SIZE_PER_RIO_THREAD ((MAX_SEND_RQ_SIZE_PER_SOCKET + MAX_RECV_RQ_SIZE_PER_SOCKET) * MAX_CLIENTS)

#define OP_RECV  1
#define OP_SEND  2

struct SOCKETINFO
{
	SOCKET socket;
	RIO_RQ	reqQueue;
	RIO_BUF rioBuffer;
	int		buff_slot;
};

map <SOCKET, SOCKETINFO> clients;
RIO_EXTENSION_FUNCTION_TABLE gRIOFuncTable = { 0, };
queue <int> availableBufQ({ 0,1,2,3,4,5,6,7,8,9 });

char* g_buffer;
RIO_BUFFERID	g_bufid;

int main()
{
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_REGISTERED_IO);

	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	::bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(SOCKADDR_IN));

	GUID functionTableId = WSAID_MULTIPLE_RIO;
	DWORD dwBytes = 0;
	if (WSAIoctl(listenSocket, SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER, &functionTableId, sizeof(GUID), (void**)&gRIOFuncTable, sizeof(gRIOFuncTable), &dwBytes, NULL, NULL))
		return false;

	listen(listenSocket, SOMAXCONN);
	RIO_CQ	compQueue = gRIOFuncTable.RIOCreateCompletionQueue(MAX_CQ_SIZE_PER_RIO_THREAD, 0);

	SOCKET clientSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_REGISTERED_IO);
	WSAOVERLAPPED overlap;
	ZeroMemory(&overlap, sizeof(overlap));
	HANDLE a_event = overlap.hEvent = WSACreateEvent();
	char acceptBuffer[2 * (sizeof(SOCKADDR_IN) + 16)];
	AcceptEx(listenSocket, clientSocket, acceptBuffer, NULL, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, NULL, &overlap);

	g_buffer = reinterpret_cast<char*>(VirtualAllocEx(GetCurrentProcess(), 0, SESSION_BUFFER_SIZE * MAX_CLIENTS, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	g_bufid = gRIOFuncTable.RIORegisterBuffer(g_buffer, SESSION_BUFFER_SIZE * MAX_CLIENTS);

	while (true) {
		if (clients.size() < MAX_CLIENTS) {
			int index = WSAWaitForMultipleEvents(1, &overlap.hEvent, FALSE, 0, FALSE);
			if (index == 0) {
				cout << "New Client [" << clientSocket << "] arrived.\n";

				WSAResetEvent(a_event);
				clients[clientSocket] = SOCKETINFO{};
				clients[clientSocket].socket = clientSocket;
				clients[clientSocket].buff_slot = availableBufQ.front();
				availableBufQ.pop();

				clients[clientSocket].reqQueue = gRIOFuncTable.RIOCreateRequestQueue(clientSocket, MAX_RECV_RQ_SIZE_PER_SOCKET, 1, MAX_SEND_RQ_SIZE_PER_SOCKET, 1,
					compQueue, compQueue, (PVOID)clientSocket);
				clients[clientSocket].rioBuffer.BufferId = g_bufid;
				clients[clientSocket].rioBuffer.Length = SESSION_BUFFER_SIZE;
				clients[clientSocket].rioBuffer.Offset = SESSION_BUFFER_SIZE * clients[clientSocket].buff_slot;
				gRIOFuncTable.RIOReceive(clients[clientSocket].reqQueue, &clients[clientSocket].rioBuffer, 1, NULL, (PVOID)OP_RECV);

				clientSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_REGISTERED_IO);
				ZeroMemory(&overlap, sizeof(overlap));
				overlap.hEvent = a_event;
				AcceptEx(listenSocket, clientSocket, acceptBuffer, NULL, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, NULL, &overlap);
			}
		}
		RIORESULT results[MAX_RIO_RESULTS];
		ULONG numResults = gRIOFuncTable.RIODequeueCompletion(compQueue, results, MAX_RIO_RESULTS);

		for (ULONG i = 0; i < numResults; ++i) {
			ULONGLONG op = results[i].RequestContext;
			ULONG bytes = results[i].BytesTransferred;
			SOCKET compSocket = static_cast<SOCKET>(results[i].SocketContext);

			if (0 == bytes) {
				cout << "Client [" << compSocket << "] closed\n";
				closesocket(clients[compSocket].socket);
				availableBufQ.push(clients[compSocket].buff_slot);
				clients.erase(compSocket);
			}
			else if (OP_RECV == op) {
				clients[compSocket].rioBuffer.Length = bytes;
				gRIOFuncTable.RIOSend(clients[compSocket].reqQueue, &clients[compSocket].rioBuffer, 1, NULL, (PVOID)OP_SEND);
			}
			else if (OP_SEND == op) {
				clients[compSocket].rioBuffer.Length = SESSION_BUFFER_SIZE;
				gRIOFuncTable.RIOReceive(clients[compSocket].reqQueue, &clients[compSocket].rioBuffer, 1, NULL, (PVOID)OP_RECV);
			}
		}
	}
	closesocket(listenSocket);
	WSACleanup();
}

