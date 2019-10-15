#include "Networks.h"
#include "ModuleNetworking.h"


static uint8 NumModulesUsingWinsock = 0;



void ModuleNetworking::reportError(const char* inOperationDesc)
{
	LPVOID lpMsgBuf;
	DWORD errorNum = WSAGetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errorNum,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	ELOG("Error %s: %d- %s", inOperationDesc, errorNum, lpMsgBuf);
}

void ModuleNetworking::disconnect()
{
	for (SOCKET socket : sockets)
	{
		shutdown(socket, 2);
		closesocket(socket);
	}

	sockets.clear();
}

bool ModuleNetworking::init()
{
	if (NumModulesUsingWinsock == 0)
	{
		NumModulesUsingWinsock++;

		WORD version = MAKEWORD(2, 2);
		WSADATA data;
		if (WSAStartup(version, &data) != 0)
		{
			reportError("ModuleNetworking::init() - WSAStartup");
			return false;
		}
	}

	return true;
}

bool ModuleNetworking::preUpdate()
{
	if (sockets.empty()) return true;

	// NOTE(jesus): You can use this temporary buffer to store data from recv()
	const uint32 incomingDataBufferSize = Kilobytes(1);
	byte incomingDataBuffer[incomingDataBufferSize];

	// TODO(jesus): select those sockets that have a read operation available
	fd_set read_fd;
	FD_ZERO(&read_fd);
	for (auto s : sockets)
		FD_SET(s, &read_fd);

	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	int result = select(0, &read_fd, nullptr, nullptr, &timeout);
	if (result == SOCKET_ERROR)
	{
		LOG("The socket couldn't be selected");
	}

	
	// TODO(jesus): for those sockets selected, check wheter or not they are
	// a listen socket or a standard socket and perform the corresponding
	// operation (accept() an incoming connection or recv() incoming data,
	// respectively).
	for (auto s : sockets)
	{
		if (FD_ISSET(s, &read_fd))
		{
			if (isListenSocket(s))
			{
				int result = accept(s, (sockaddr*)&address, (int*)sizeof(address));
				if (result >= 0)
				{
					// On accept() success, communicate the new connected socket to the
					// subclass (use the callback onSocketConnected()), and add the new
					// connected socket to the managed list of sockets.
					onSocketConnected(s, address);
				}
				else
					LOG("Couldn't accept a socket");
			}
			else
			{
				char data[69420];
				int result = recvfrom(s, data, 69420, 0, (sockaddr*)&address, (int*)sizeof(address));
				if (result > 0)
				{
					// On recv() success, communicate the incoming data received to the
					// subclass (use the callback onSocketReceivedData()).
					onSocketReceivedData(s, (byte*)data);
				}
				else
					// TODO(jesus): handle disconnections. Remember that a socket has been
					// disconnected from its remote end either when recv() returned 0,
					// or when it generated some errors such as ECONNRESET.
					// Communicate detected disconnections to the subclass using the callback
					// onSocketDisconnected().
					onSocketDisconnected(s);
			}
		}
	}

	// TODO(jesus): Finally, remove all disconnected sockets from the list
	// of managed sockets.
	disconnect();

	return true;
}

bool ModuleNetworking::cleanUp()
{
	disconnect();

	NumModulesUsingWinsock--;
	if (NumModulesUsingWinsock == 0)
	{

		if (WSACleanup() != 0)
		{
			reportError("ModuleNetworking::cleanUp() - WSACleanup");
			return false;
		}
	}

	return true;
}

void ModuleNetworking::addSocket(SOCKET socket)
{
	sockets.push_back(socket);
}
