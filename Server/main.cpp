#include <iostream>
#include <WS2tcpip.h> // includes winsock.h
#include <string>
#include <thread>
#include <vector>

#pragma comment (lib, "ws2_32.lib") // tells the linker to add library to dependiences

#define BUF_LENGTH 512

const int MAX_CLIENTS = 3;

using namespace std;

struct Client
{
	int id;
	SOCKET socket;
	string nick;
};

int main(int argc, char *argv[])
{
	// init winsock
	WSADATA wsaData;
	cout << "Initializing winsock.\n";
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		cout << "Can't initialize winsock. Aborting.\n";
		return 1;
	}

	// setup hint
	addrinfo hint; // hold host address and socket type information
	ZeroMemory(&hint, sizeof(hint));
	hint.ai_family = AF_INET; // IPv4 address family
	hint.ai_socktype = SOCK_STREAM; // socket type for TCP
	hint.ai_protocol = IPPROTO_TCP; // 
	hint.ai_flags = AI_PASSIVE; // address used in bind()

	// setup server
	addrinfo *server = NULL;
	cout << "Setting up server.\n";
	getaddrinfo(static_cast<LPCTSTR>(argv[1]), argv[2], &hint, &server); //argv[1] - IP, argv[2] - port

	// create listening socket
	SOCKET server_socket = INVALID_SOCKET;
	cout << "Creating server socket.\n";
	server_socket = socket(server->ai_family, server->ai_socktype, server->ai_protocol);

	// setup socket options
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, "1", sizeof(int)); // not required

	// bind address to socket
	std::cout << "Binding socket.\n" << std::endl;
	bind(server_socket, server->ai_addr, (int)server->ai_addrlen);

	// listen for incoming connections
	std::cout << "Listening.\n" << std::endl;
	listen(server_socket, SOMAXCONN);

	// initialize the client list
	vector<Client> clients(MAX_CLIENTS);

	for (int i = 0; i < MAX_CLIENTS; i++)
		clients[i] = { -1, INVALID_SOCKET };


	// wait for requests
	string msg = "";
	int num_clients = 0;
	int temp_id = -1;

	while (1)
	{
		SOCKET incoming = INVALID_SOCKET;
		incoming = accept(server_socket, NULL, NULL);

		if (incoming == INVALID_SOCKET)
			continue;

		num_clients = 0;
		temp_id = -1;

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (clients[i].socket == INVALID_SOCKET && temp_id == -1)
			{
				clients[i].socket = incoming;
				clients[i].id = i;
				temp_id = i;
			}

			if (clients[i].socket != INVALID_SOCKET)
				num_clients++;
		}

		if (temp_id != -1)
		{
			// receive nick from client
			// send the id to client
			// create thread process which receives message and sends it to other clients
		}
		else
		{

		}
	}

	// close listening socket
	closesocket(server_socket);

	// close client sockets
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		closesocket(clients[i].socket);
	}

	// cleanup winsock
	WSACleanup();

	system("PAUSE");
}

