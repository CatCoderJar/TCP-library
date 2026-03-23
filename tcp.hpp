#define NOT_INITIALIZED -512
#include <iostream>
#include <Winsock2.h>
#include <WS2tcpip.h>
#include <optional>
#include <vector>
#pragma comment(lib, "Ws2_32.lib")

class tcpServer
{
private:
	bool isSafeToContinue{ false };
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
public:
	std::vector<SOCKET> clients{};
	tcpServer(std::string_view ipArg, std::string_view portArg, std::size_t clientsLimitArg = SOMAXCONN) : ip(ipArg), port(portArg), clientsLimit(clientsLimitArg)
	{

	}

	void shutdownServer()
	{
		if (!isSafeToContinue) { return; }
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
		isSafeToContinue = true;
		return 0;
	}
	int acceptClients(int clientsLimit = SOMAXCONN, sockaddr* addr = NULL, int* addrLen = NULL) // loop, it will break if error and return error code
	{
		if (!isSafeToContinue) { return NOT_INITIALIZED; }
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
		if (!isSafeToContinue) { return NOT_INITIALIZED; }
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

	int sendMsg(std::string message, bool encrypt)
	{
		if (!isSafeToContinue) { return NOT_INITIALIZED; }
		if (encrypt)
		{
			for (std::size_t i{ 0 }; i < message.size(); i++)
			{
				message[i] ^= key;
			}
		}
		if (send(ClientSocket, message.data(), message.length(), 0) <= 0)
		{
			return WSAGetLastError();
		}

		return 0;
	}

	int sendMsgToSpecificClient(std::string message, std::size_t clientIndex, bool encrypt = false)
	{
		if (!isSafeToContinue) { return NOT_INITIALIZED; }
		if (encrypt)
		{
			for (std::size_t i{ 0 }; i < message.size(); i++)
			{
				message[i] ^= key;
			}
		}
		if (send(clients[clientIndex], message.c_str(), message.length(), 0) <= 0)
		{
			return WSAGetLastError();
		}

		return 0;
	}

	std::optional<std::string> readData(bool decrypt = false) // Make as a cycle, better as thread, will return std::nullopt if connection will be closed, or if server won't be initialized
	{
		if (!isSafeToContinue) { return std::nullopt; }
		int res{ 0 };
		std::optional<std::string> data{};

		ZeroMemory(buff, sizeof(buff));
		res = recv(ClientSocket, buff, 1000000, 0);
		if (res == 0 || res < 0)
		{
			return std::nullopt;
		}
		data = buff;

		if (data.has_value())
		{
			if (decrypt)
			{
				for (std::size_t i{ 0 }; i < data.value().size(); i++)
				{
					data.value()[i] ^= key;
				}
			}
			return data.value();
		}
		else
		{
			return std::nullopt;
		}
	}

	std::optional<std::string> readDataFromSpecialClient(std::size_t clientIndex, bool decrypt = false) // Make as a cycle, better as thread, will return std::nullopt if connection will be closed, or if server won't be initialized
	{
		if (!isSafeToContinue) { return std::nullopt; }
		int res{ 0 };
		std::optional<std::string> data{};

		ZeroMemory(buff, sizeof(buff));
		res = recv(clients[clientIndex], buff, 1000000, 0);
		if (res == 0 || res < 0)
		{
			return std::nullopt;
		}
		data = buff;

		if (data.has_value())
		{
			if (decrypt)
			{
				for (std::size_t i{ 0 }; i < data.value().size(); i++)
				{
					data.value()[i] ^= key;
				}
			}
			return data.value();
		}
		else
		{
			return std::nullopt;
		}
	}
};

class tcpClient
{
private:
	bool isSafeToContinue{ false };
	char key{ 'h' };
	char buff[1000000]{};
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
		if (!isSafeToContinue) { return; }
		closesocket(ConnectSocket);
		freeaddrinfo(addrResult);
		WSACleanup();
	}

	int initClient()
	{
		if (WSAStartup(ver, &data) != 0)
		{
			return WSAGetLastError();
		}

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		if (getaddrinfo(ip.c_str(), port.c_str(), &hints, &addrResult) != 0)
		{
			freeaddrinfo(addrResult);
			WSACleanup();

			return WSAGetLastError();
		}

		ConnectSocket = socket(addrResult->ai_family, addrResult->ai_socktype, (int)addrResult->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET)
		{
			freeaddrinfo(addrResult);
			WSACleanup();
			return WSAGetLastError();
		}

		return 0;
	}

	void connectLoop() // While isn't connected, trying to connect server
	{
		if (!isSafeToContinue) { return; }
		while (connect(ConnectSocket, addrResult->ai_addr, addrResult->ai_addrlen) == SOCKET_ERROR) { continue; }
	}

	int connectLoop(std::string_view ipArg, std::string_view portArg) // experimental
	{
		if (!isSafeToContinue) { return NOT_INITIALIZED; }
		if (!addrResult)
		{
			freeaddrinfo(addrResult);
		}
		ZeroMemory(&hints, sizeof(hints));
		if (getaddrinfo(ipArg.data(), portArg.data(), &hints, &addrResult) != 0)
		{
			return WSAGetLastError();
		}
		return connect(ConnectSocket, addrResult->ai_addr, addrResult->ai_addrlen);
	}

	int connectToServer() // just default connect
	{
		if (!isSafeToContinue) { return NOT_INITIALIZED; }
		return connect(ConnectSocket, addrResult->ai_addr, addrResult->ai_addrlen);
	}

	int connectToServer(std::string_view ipArg, std::string_view portArg) // experimental
	{
		if (!isSafeToContinue) { return NOT_INITIALIZED; }
		if (!addrResult)
		{
			freeaddrinfo(addrResult);
		}
		ZeroMemory(&hints, sizeof(hints));
		if (getaddrinfo(ipArg.data(), portArg.data(), &hints, &addrResult) != 0)
		{
			return WSAGetLastError();
		}
		return connect(ConnectSocket, addrResult->ai_addr, addrResult->ai_addrlen);
	}

	int sendMsg(std::string message, bool encrypt)
	{
		if (!isSafeToContinue) { return NOT_INITIALIZED; }
		if (encrypt)
		{
			for (std::size_t i{ 0 }; i < message.size(); i++)
			{
				message[i] ^= key;
			}
		}
		if (send(ConnectSocket, message.data(), message.length(), 0) < 0)
		{
			return WSAGetLastError();
		}

		return 0;
	}

	std::optional<std::string> readData(bool decrypt = false) // Make as a cycle, better as thread, will return std::nullopt if connection will be closed, or client isn't initialized
	{
		if (!isSafeToContinue) { return std::nullopt; }
		int res{ 0 };
		std::optional<std::string> data{};

		ZeroMemory(buff, sizeof(buff));
		res = recv(ConnectSocket, buff, 1000000, 0);
		if (res == 0 || res < 0)
		{
			return std::nullopt;
		}
		data = buff;

		if (data.has_value())
		{
			if (decrypt)
			{
				for (std::size_t i{ 0 }; i < data.value().size(); i++)
				{
					data.value()[i] ^= key;
				}
			}
			return data.value();
		}
		else
		{
			return std::nullopt;
		}
	}
};
