#include "Networks.h"
#include "ModuleNetworking.h"
#include <list>

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
		printWSErrorAndExit("The socket couldn't be selected");
	}

	
	std::list<SOCKET> disconnectedSockets;

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
				addressLen = sizeof(address);
				SOCKET newSocket = accept(s, (sockaddr*)&address, &addressLen);
				if (newSocket)
				{
					// On accept() success, communicate the new connected socket to the
					// subclass (use the callback onSocketConnected()), and add the new
					// connected socket to the managed list of sockets.
					onSocketConnected(newSocket, address);
					addSocket(newSocket);
				}
				else
					printWSErrorAndExit("Couldn't accept a socket");
			}
			else
			{
				InputMemoryStream packet;
				int result = recv(s, packet.GetBufferPtr(), packet.GetCapacity(), 0);
				if (result > 0)
				{
					// On recv() success, communicate the incoming data received to the
					// subclass (use the callback onSocketReceivedData()).
					packet.SetSize((uint32)result);
					onSocketReceivedData(s, packet);
				}
				else if (result == 0 || result == ECONNRESET)
				{
					// TODO(jesus): handle disconnections. Remember that a socket has been
					// disconnected from its remote end either when recv() returned 0,
					// or when it generated some errors such as ECONNRESET.
					// Communicate detected disconnections to the subclass using the callback
					// onSocketDisconnected().
					onSocketDisconnected(s);
					disconnectedSockets.push_back(s);
				}
				else
				{
					printWSErrorAndExit("Error recieving data from socket");
				}
			}
		}
	}


	// TODO(jesus): Finally, remove all disconnected sockets from the list
	// of managed sockets.

	for (auto disc_sock : disconnectedSockets)
		sockets.erase(std::remove(sockets.begin(), sockets.end(), disc_sock), sockets.end());

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

bool ModuleNetworking::sendPacket(const OutputMemoryStream & packet, SOCKET socket)
{
	if (send(socket, packet.GetBufferPtr(), packet.GetSize(), 0) == SOCKET_ERROR)
	{
		reportError("Couldn't send packet");
		return false;
	}
	return true;
}

void ModuleNetworking::addSocket(SOCKET socket)
{
	sockets.push_back(socket);
}
