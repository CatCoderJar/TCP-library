#include <iostream>
#include <Winsock2.h>
#include <WS2tcpip.h>
#include <optional>
#pragma comment(lib, "Ws2_32.lib")

class tcpServer
{
private:
	WSADATA data;
	ADDRINFO hints;
	ADDRINFO* addrResult;
	SOCKET ListenSocket{ INVALID_SOCKET };
	SOCKET ClientSocket{ INVALID_SOCKET };
	WORD ver{ MAKEWORD(2, 2) };
	std::string ip;
	std::string port;
public:
	tcpServer(std::string_view ipArg, std::string_view portArg) : ip(ipArg), port(portArg)
	{

	}

	void shutdownServer()
	{
		closesocket(ListenSocket);
		freeaddrinfo(addrResult);
		WSACleanup();
	}

	int startServer()
	{
		if (WSAStartup(ver, &data) != 0)
		{
			std::cout << std::format("WSAStartup error, code:{}\n", WSAGetLastError());
			return -1;
		}

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		if (getaddrinfo(ip.c_str(), port.c_str(), &hints, &addrResult) != 0)
		{
			std::cout << std::format("Getaddrinfo error, code:{}\n", WSAGetLastError());
			WSACleanup();

			return -1;
		}

		ListenSocket = socket(addrResult->ai_family, addrResult->ai_socktype, (int)addrResult->ai_protocol);
		if (ListenSocket == INVALID_SOCKET)
		{
			std::cout << std::format("Socket creation error error, code:{}\n", WSAGetLastError());

			freeaddrinfo(addrResult);
			WSACleanup();

			return -1;
		}

		if (bind(ListenSocket, addrResult->ai_addr, addrResult->ai_addrlen) == SOCKET_ERROR)
		{
			std::cout << std::format("Bind error, code:{}\n", WSAGetLastError());

			closesocket(ListenSocket);
			freeaddrinfo(addrResult);
			WSACleanup();

			return -1;
		}

		if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
		{
			std::cout << std::format("Listen error, code:{}\n", WSAGetLastError());

			freeaddrinfo(addrResult);
			WSACleanup();

			return -1;
		}

		return 0;
	}
	int acceptSocket()
	{
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET)
		{
			std::cout << std::format("Accept error, code:{}\n", WSAGetLastError());

			closesocket(ListenSocket);
			freeaddrinfo(addrResult);
			WSACleanup();

			return -1;
		}

		return 0;
	}
	int sendMsg(std::string message)
	{
		if (send(ClientSocket, message.c_str(), message.length(), 0) <= 0)
		{
			return WSAGetLastError();
		}

		return 0;
	}
	std::optional<std::string> readData() // Make as loop with thread, will return std::nullopt if connection will be closed
	{
		int res{ 0 };
		char* buff = new char[1000000];
		std::string data{};

		ZeroMemory(buff, sizeof(buff));
		res = recv(ClientSocket, buff, 1000000, 0);
		if (res == 0)
		{
			return std::nullopt;
		}
		data = buff;

		delete[] buff;

		return data;
	}
};

class tcpClient
{
private:
	char* buff = new char[1000000];
	std::string ip;
	std::string port;
	WSADATA data;
	ADDRINFO hints;
	ADDRINFO* addrResult;
	SOCKET ConnectSocket{ INVALID_SOCKET };
	WORD ver{ MAKEWORD(2, 2) };
public:
	tcpClient(std::string ipArg, std::string portArg) : ip(ipArg), port(portArg)
	{

	}

	void shutdownClient()
	{
		closesocket(ConnectSocket);
		freeaddrinfo(addrResult);
		WSACleanup();
	}

	int startClient()
	{
		if (WSAStartup(ver, &data) != 0)
		{
			std::cout << std::format("WSAStartup error, code:{}\n", WSAGetLastError());
			return -1;
		}

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		if (getaddrinfo("localhost", "4096", &hints, &addrResult) != 0)
		{
			std::cout << std::format("Getaddrinfo error, code:{}\n", WSAGetLastError());

			freeaddrinfo(addrResult);
			WSACleanup();

			return -1;
		}

		ConnectSocket = socket(addrResult->ai_family, addrResult->ai_socktype, (int)addrResult->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET)
		{
			std::cout << std::format("Socket creation error error, code:{}\n", WSAGetLastError());
			freeaddrinfo(addrResult);
			WSACleanup();
			return -1;
		}

		return 0;
	}

	void connectToServer()
	{
		while (connect(ConnectSocket, addrResult->ai_addr, addrResult->ai_addrlen) == SOCKET_ERROR) { continue; }
		std::cout << std::format("Connection established with at {} and port {}", ip, port);
	}

	int sendMsg(std::string message)
	{
		if (send(ConnectSocket, message.c_str(), message.length(), 0) != 0)
		{
			return -1;
		}

		return 0;
	}

	std::optional<std::string> readData() // Make as a cycle, will return std::nullopt if connection will be closed
	{
		int res{ 0 };
		std::string data{};

		ZeroMemory(buff, sizeof(buff));
		res = recv(ConnectSocket, buff, 1000000, 0);
		if (res == 0)
		{
			return std::nullopt;
		}
		data = buff;

		delete[] buff;

		return data;
	}
};
