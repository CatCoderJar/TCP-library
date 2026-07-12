#include <iostream>
#include <Winsock2.h>
#include <WS2tcpip.h>
#include <optional>
#include <vector>
#pragma comment(lib, "Ws2_32.lib")

class tcpServer
{
public:
	char key{ 'h' };
	char buff[1000000]{};
	WSADATA data;
	ADDRINFO hints;
	ADDRINFO* addrResult;
	SOCKET ListenSocket{ INVALID_SOCKET };
	SOCKET ClientSocket{ INVALID_SOCKET };
	WORD ver{ MAKEWORD(2, 2) };
	std::size_t clientsLimit{ SOMAXCONN };
	std::string ip;
	std::string port;

	std::vector<SOCKET> clients{};
	tcpServer(std::string_view ipArg, std::string_view portArg, std::size_t clientsLimitArg) : ip(ipArg), port(portArg), clientsLimit(clientsLimitArg)
	{

	}

	void shutdownServer()
	{
		if (ListenSocket)
		{
			closesocket(ListenSocket);
		}

		for (std::size_t i{ 0 }; i < clients.size(); i++)
		{
			closesocket(clients[i]);
		}

		freeaddrinfo(addrResult);
		WSACleanup();
	}


	int initServer()
	{
		if (WSAStartup(ver, &data) != 0)
		{
			return WSAGetLastError();
		}

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		if (getaddrinfo(ip.c_str(), port.c_str(), &hints, &addrResult) != 0)
		{
			WSACleanup();

			return WSAGetLastError();
		}

		ListenSocket = socket(addrResult->ai_family, addrResult->ai_socktype, (int)addrResult->ai_protocol);
		if (ListenSocket == INVALID_SOCKET)
		{
			// std::cout << std::format("Socket creation error error, code:{}\n", WSAGetLastError());

			freeaddrinfo(addrResult);
			WSACleanup();

			return WSAGetLastError();
		}

		if (bind(ListenSocket, addrResult->ai_addr, addrResult->ai_addrlen) == SOCKET_ERROR)
		{

			closesocket(ListenSocket);
			freeaddrinfo(addrResult);
			WSACleanup();

			return WSAGetLastError();
		}

		if (listen(ListenSocket, clientsLimit) == SOCKET_ERROR)
		{
			freeaddrinfo(addrResult);
			WSACleanup();

			return WSAGetLastError();
		}

		return 0;
	}
	int acceptClients(sockaddr* addr = NULL, int* addrLen = NULL) // loop, it will break if error and return error code
	{
		for (std::size_t i{ 0 }; i <= clientsLimit; i++)
		{
			clients[i] = accept(ListenSocket, addr, addrLen);

			if (clients[i] == INVALID_SOCKET)
			{
				return WSAGetLastError();
			}
		}

		return 0;
	}

	int acceptClient(sockaddr* addr = NULL, int* addrLen = NULL)
	{
		ClientSocket = accept(ListenSocket, addr, addrLen);
		if (ClientSocket == INVALID_SOCKET)
		{
			// std::cout << std::format("Accept error, code:{}\n", WSAGetLastError());

			closesocket(ListenSocket);
			freeaddrinfo(addrResult);
			WSACleanup();

			return WSAGetLastError();
		}

		return 0;
	}

	int sendMsg(char* buff, int size)
	{
		if (send(ClientSocket, buff, size, 0) <= 0)
		{
			return WSAGetLastError();
		}

		return 0;
	}

	virtual int sendMsgV() {}

	int sendMsgToSpecificClient(char* buff, int size, std::size_t clientIndex, bool encrypt = false)
	{
		if (send(clients[clientIndex], buff, size, 0) <= 0)
		{
			return WSAGetLastError();
		}

		return 0;
	}

	char* readData() // Make as a cycle, better as thread, will return std::nullopt if connection will be closed
	{
		int res{ 0 };

		ZeroMemory(buff, sizeof(buff));
		res = recv(ClientSocket, buff, 1000000, 0);
		if (res == 0)
		{
			return 0;
		}
		return buff;
	}

	virtual int readDataV() {}

	virtual char* readDataFromSpecialClientV() {}

	char* readDataFromSpecialClient(std::size_t clientIndex) // Make as a cycle, better as thread, will return std::nullopt if connection will be closed
	{
		int res{ 0 };


		ZeroMemory(buff, sizeof(buff));
		res = recv(clients[clientIndex], buff, 1000000, 0);
		if (res == 0)
		{
			return 0;
		}
		return buff;
	}
};

class tcpClient
{
public:
	char buff[1000000]{};
	std::string ip;
	std::string port;
	WSADATA data;
	ADDRINFO hints;
	ADDRINFO* addrResult;
	SOCKET ConnectSocket{ INVALID_SOCKET };
	WORD ver{ MAKEWORD(2, 2) };
	tcpClient(std::string ipArg, std::string portArg) : ip(ipArg), port(portArg)
	{

	}

	void shutdownClient()
	{
		closesocket(ConnectSocket);
		freeaddrinfo(addrResult);
		WSACleanup();
	}

	int initClient()
	{
		if (WSAStartup(ver, &data) != 0)
		{
			// std::cout << std::format("WSAStartup error, code:{}\n", WSAGetLastError());
			return WSAGetLastError();
		}

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		if (getaddrinfo(ip.c_str(), port.c_str(), &hints, &addrResult) != 0)
		{
			// std::cout << std::format("Getaddrinfo error, code:{}\n", WSAGetLastError());

			freeaddrinfo(addrResult);
			WSACleanup();

			return WSAGetLastError();
		}

		ConnectSocket = socket(addrResult->ai_family, addrResult->ai_socktype, (int)addrResult->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET)
		{
			// std::cout << std::format("Socket creation error error, code:{}\n", WSAGetLastError());
			freeaddrinfo(addrResult);
			WSACleanup();
			return WSAGetLastError();
		}

		return 0;
	}

	void connectLoop() // While isn't connected, trying to connect server
	{
		while (connect(ConnectSocket, addrResult->ai_addr, addrResult->ai_addrlen) == SOCKET_ERROR) { Sleep(50); }
	}

	int connectToServer() // just default connect
	{
		return connect(ConnectSocket, addrResult->ai_addr, addrResult->ai_addrlen);
	}

	int sendMsg(char* buff, int size)
	{
		if (send(ConnectSocket, buff, size, 0) <= 0)
		{
			return WSAGetLastError();
		}

		return 0;
	}

	virtual int sendMsgV() {}

	char* readData() // Make as a cycle, better as thread, will return std::nullopt if connection will be closed
	{
		int res{ 0 };

		ZeroMemory(buff, sizeof(buff));
		res = recv(ConnectSocket, buff, 1000000, 0);
		if (res == 0)
		{
			return 0;
		}
		return buff;
	}

	virtual char* readDataV() {}
};
