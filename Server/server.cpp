#include <iostream>
#include <WS2tcpip.h> // includes winsock.h
#include <string>
#include <thread>
#include <vector>

#pragma comment (lib, "ws2_32.lib") // tells the linker to add library to dependiences

#define BUF_LENGTH 512

const int MAX_CLIENTS = 5; // TO DO: retire from this

struct Client
{
	int id;
	SOCKET socket;
	std::string nick;
};

int thread_client(Client &new_client, std::vector<Client> &clients, std::thread &thread);

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		std::cout << "Run Server with 2 parameters: IP address and port.\n";
		return 1;
	}

	// init winsock
	WSADATA wsaData;
	addrinfo hint; // hold host address and socket type information
	addrinfo *server = NULL;
	SOCKET server_socket = INVALID_SOCKET;
	std::vector<Client> clients(MAX_CLIENTS);
	//std::vector<Client> clients;
	std::thread threads[MAX_CLIENTS]; //vector instead of an array
	//std::vector<std::thread> threads;

	std::string msg = "";
	int num_clients = 0;
	int temp_id = -1;

	std::cout << "Initializing winsock.\n";
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cout << "Can't initialize winsock. Aborting.\n";
		return 1;
	}

	// setup hint
	ZeroMemory(&hint, sizeof(hint));
	hint.ai_family = AF_INET; // IPv4 address family
	hint.ai_socktype = SOCK_STREAM; // socket type for TCP
	hint.ai_protocol = IPPROTO_TCP; // 
	hint.ai_flags = AI_PASSIVE; // address used in bind()

	// setup server
	std::cout << "Setting up server.\n";
	getaddrinfo(static_cast<LPCTSTR>(argv[1]), argv[2], &hint, &server); //argv[1] - IP, argv[2] - port

	// create listening socket
	std::cout << "Creating server socket.\n";
	server_socket = socket(server->ai_family, server->ai_socktype, server->ai_protocol);

	// setup socket options
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, "1", sizeof(int));
	//setsockopt(server_socket, IPPROTO_TCP, TCP_NODELAY, "1", sizeof(int));

	// bind address to socket
	std::cout << "Binding socket.\n";
	bind(server_socket, server->ai_addr, (int)server->ai_addrlen);

	// listen for incoming connections
	std::cout << "Listening.\n";
	listen(server_socket, SOMAXCONN);

	// initialize the client list
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		clients[i] = { -1, INVALID_SOCKET };
	}
 
	// wait for requests
	while (1)
	{
		SOCKET incoming = INVALID_SOCKET;
		incoming = accept(server_socket, NULL, NULL);

		if (incoming == INVALID_SOCKET) continue;

		//Reset the number of clients
		num_clients = -1;

		//Create a temporary id for the next client
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
			char nickname[BUF_LENGTH] = "";
			recv(clients[temp_id].socket, nickname, BUF_LENGTH, 0);
			clients[temp_id].nick = nickname;

			// send the id to client
			std::cout << clients[temp_id].nick << " connected to the server" << std::endl;
			msg = std::to_string(clients[temp_id].id);
			send(clients[temp_id].socket, msg.c_str(), strlen(msg.c_str()), 0);

			// create thread process for every client which receives message from this client and sends it to other clients
			threads[temp_id] = std::thread(thread_client, std::ref(clients[temp_id]), std::ref(clients), std::ref(threads[temp_id]));
		}
		else
		{
			msg = "Server is full";
			send(incoming, msg.c_str(), strlen(msg.c_str()), 0);
			std::cout << msg << std::endl;
		}
	}

	// close listening socket
	closesocket(server_socket);

	// close client sockets
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		threads[i].detach();
		closesocket(clients[i].socket);
	}

	// cleanup winsock
	WSACleanup();

	system("PAUSE");
	return 0;
}

int thread_client(Client &new_client, std::vector<Client> &clients, std::thread &thread)
{
	std::string msg = "";
	char tempmsg[BUF_LENGTH] = "";

	while (1)
	{
		memset(tempmsg, 0, BUF_LENGTH); //?

		if (new_client.socket != 0)
		{
			int iResult = recv(new_client.socket, tempmsg, BUF_LENGTH, 0);

			if (iResult != SOCKET_ERROR)
			{
				if (strcmp("", tempmsg))
					msg = "<" + new_client.nick + "> " + tempmsg;

				std::cout << msg.c_str() << std::endl;

				// broadcast message to other clients
				for (int i = 0; i < MAX_CLIENTS; i++)
				{
					if (clients[i].socket != INVALID_SOCKET)
						if (new_client.id != i)
							iResult = send(clients[i].socket, msg.c_str(), strlen(msg.c_str()), 0);
				}

			}
			else
			{
				msg = new_client.nick + " has disconnected from the server.";
				std::cout << msg << std::endl;

				closesocket(new_client.socket);
				closesocket(clients[new_client.id].socket);
				clients[new_client.id].socket = INVALID_SOCKET;

				// broadcast the disconnection message to other clients
				for (int i = 0; i < MAX_CLIENTS; i++)
				{
					if (clients[i].socket != INVALID_SOCKET)
						iResult = send(clients[i].socket, msg.c_str(), strlen(msg.c_str()), 0);
				}

				break;
			}
		}
	}

	thread.detach();

	return 0;
}