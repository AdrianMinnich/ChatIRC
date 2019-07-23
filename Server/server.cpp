#include <iostream>
#include <WS2tcpip.h> // includes winsock.h
#include <string>
#include <thread>
#include <vector>

#pragma comment (lib, "ws2_32.lib") // tells the linker to add library to dependiences

#define BUF_LENGTH 512

const int MAX_CLIENTS = 100; // TO DO: retire from this

struct Client
{
	int id;
	SOCKET socket;
	std::string nick;
};

int thread_client(Client &new_client, std::vector<Client> &clients);

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		std::cout << "Run Server with 1 parameter: Listening port. For example: Server.exe \"3504\"\n";
		system("PAUSE");
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
	getaddrinfo(NULL, argv[1], &hint, &server); //argv[1] - port

	// create listening socket
	std::cout << "Creating server socket.\n";
	server_socket = socket(server->ai_family, server->ai_socktype, server->ai_protocol);

	// setup socket options
	int OPTION_VALUE = 1; // setsockopt
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &OPTION_VALUE, sizeof(int)); // SOL_SOCKET - all, SO_REUSEADDR - zezwala na ponowne wykorzystanie tego samego numeru portu przez gniazdo bez koniecznosci odczekiwania czasu okreslonego stala TCP_TIMEWAIT_LEN
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

		if (incoming == INVALID_SOCKET) 
			continue;

		temp_id++;

		clients[temp_id].socket = incoming;
		clients[temp_id].id = temp_id;

		num_clients++;
		
		// create thread process for every client which receives message from this client and sends it to other clients
		threads[temp_id] = std::thread(thread_client, std::ref(clients[temp_id]), std::ref(clients)); //pushback
		
	}

	// close listening socket
	closesocket(server_socket);

	// close client sockets
	for (int i = 0; i < num_clients; i++)
	{
		threads[i].detach();
		closesocket(clients[i].socket);
	}

	// cleanup winsock
	WSACleanup();

	system("PAUSE");
	return 0;
}

int thread_client(Client &new_client, std::vector<Client> &clients)
{
	std::string msg = "";
	char tempmsg[BUF_LENGTH] = ""; 

	// receive nick from client
	char nickname[BUF_LENGTH] = "";
	recv(new_client.socket, nickname, BUF_LENGTH, 0);
	new_client.nick = nickname;

	// send the id to client
	std::cout << new_client.nick << " connected to the server ID: " << new_client.id << std::endl;
	msg = std::to_string(new_client.id);
	send(new_client.socket, msg.c_str(), strlen(msg.c_str()), 0);

	while (1)
	{		
		memset(tempmsg, 0, BUF_LENGTH);

		if (new_client.socket != 0)
		{
			int iResult = recv(new_client.socket, tempmsg, BUF_LENGTH, 0);

			if (iResult != SOCKET_ERROR)
			{
				if (strcmp("", tempmsg))
					msg = "<" + new_client.nick + "> " + tempmsg;

				std::cout << msg.c_str() << std::endl;

				// broadcast message to other clients co jesli jednoczesnie??? 1 watek jest wlascicielem 1 socketa tylko on operuje na tym sockecie. dac do kolejki i watki patrza u siebie i wysylaja u siebie. komunikacja miedzy watkami. 
				for (int i = 0; i < (signed) clients.size(); i++)
				{
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
				for (int i = 0; i < (signed) clients.size(); i++)
				{
					iResult = send(clients[i].socket, msg.c_str(), strlen(msg.c_str()), 0);
				}
				break;
			}
		}
	}

	return 0;
}