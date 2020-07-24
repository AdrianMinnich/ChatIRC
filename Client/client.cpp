#include <iostream>
#include <ws2tcpip.h>
#include <string>
#include <thread>

#pragma comment (lib, "Ws2_32.lib")

#define BUF_LENGTH 512           

struct Client
{
	SOCKET socket;
	int id;
	char receivedMessage[BUF_LENGTH];
	std::string nick;
};

int process_client(Client &new_client);

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		std::cout << "Run Client with 3 parameters: IP address, port and your nick. For example: Client.exe \"192.168.100.50\" \"3504\" \"A\"\n";
		system("PAUSE");
		return 1;
	}

	// init winsock
	WSADATA wsaData;
	std::cout << "Starting client.\n";
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		std::cout << "WSAStartup() failed with error: " << iResult << std::endl;
		system("PAUSE");
		return 1;
	}

	// setup hint
	addrinfo hint;
	ZeroMemory(&hint, sizeof(hint));
	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = IPPROTO_TCP;

	std::cout << "Connecting.\n";

	// setup connection to server
	addrinfo *result = NULL;
	iResult = getaddrinfo(static_cast<LPCTSTR>(argv[1]), argv[2], &hint, &result); //argv[1] - IP, argv[2] - port
	if (iResult != 0)
	{
		std::cout << "getaddrinfo() failed with error: " << iResult << std::endl;
		WSACleanup();
		system("PAUSE");
		return 1;
	}

	// init client
	Client client = { INVALID_SOCKET, -1, "", argv[3] }; // init client

	// attempt to connect to an address until success
	addrinfo *ptr = NULL;
	for (ptr = result; ptr != NULL; ptr->ai_next) // going through linked list until end
	{
		// create a socket for connecting to server
		client.socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (client.socket == INVALID_SOCKET)
		{
			std::cout << "socket() failed with error: " << WSAGetLastError() << std::endl;
			WSACleanup();
			system("PAUSE");
			return 1;
		}

		// connect to server
		iResult = connect(client.socket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR)
		{
			closesocket(client.socket);
			client.socket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (client.socket == INVALID_SOCKET)
	{
		std::cout << "Unable to connect to server.\n";
		WSACleanup();
		system("PAUSE");
		return 1;
	}

	// send nick to server
	std::cout << "Successfully connected. " << client.nick << " has joined the server.";
	send(client.socket, client.nick.c_str(), strlen(client.nick.c_str()), 0);

	// receive id from server
	recv(client.socket, client.receivedMessage, BUF_LENGTH, 0);
	std::string message = client.receivedMessage;
	std::string sent_message;
	
	client.id = atoi(client.receivedMessage);

	std::cout << " ID: " << client.id << std::endl;

	// create thread process which receives messages from server
	std::thread tThread(process_client, std::ref(client));

	// loop where messages can be send
	while (1)
	{
		std::getline(std::cin, sent_message);

		if (sent_message.length() > BUF_LENGTH)
		{
			std::cout << "Can't send message beacuse it's too long.\n";
			continue;
		}

		iResult = send(client.socket, sent_message.c_str(), strlen(sent_message.c_str()), 0);

		if (iResult <= 0)
		{
			std::cout << "send() failed with error: " << WSAGetLastError() << std::endl;
			break;
		}
	}

	// shutdown connection
	tThread.detach();
	
	std::cout << "Shutting down socket.\n";
	iResult = shutdown(client.socket, SD_SEND);
	if (iResult == SOCKET_ERROR)
	{
		std::cout << "shutdown() failed with error: " << WSAGetLastError() << std::endl;
		closesocket(client.socket);
		WSACleanup();
		system("PAUSE");
		return 1;
	}

	closesocket(client.socket);
	WSACleanup();
	system("PAUSE");
}

int process_client(Client &new_client)
{
	while (1)
	{
		memset(new_client.receivedMessage, 0, BUF_LENGTH);

		if (new_client.socket != 0)
		{
			int iResult = recv(new_client.socket, new_client.receivedMessage, BUF_LENGTH, 0);

			if (iResult != SOCKET_ERROR)
				std::cout << new_client.receivedMessage << std::endl;
			else
			{
				std::cout << "recv() failed: " << WSAGetLastError() << std::endl;
				break;
			}
		}
	}

	if (WSAGetLastError() == WSAECONNRESET)
		std::cout << "The server has disconnected\n";

	return 0;
}