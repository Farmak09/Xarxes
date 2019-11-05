#include "ModuleNetworkingServer.h"




//////////////////////////////////////////////////////////////////////
// ModuleNetworkingServer public methods
//////////////////////////////////////////////////////////////////////

bool ModuleNetworkingServer::start(int port)
{
	// TODO(jesus): TCP listen socket stuff
	// - Create the listenSocket
	listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET) {
		printWSErrorAndExit("Server socket couldn't be created");
	}

	// - Set address reuse
	int enable = 1;
	int result = setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(int));
	if (result == SOCKET_ERROR) {
		printWSErrorAndExit("Couldn't ser address to reuse");
	}

	// - Bind the socket to a local interface
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET; // IPv4
	serverAddr.sin_addr.S_un.S_addr = INADDR_ANY; // Any address
	serverAddr.sin_port = htons(port); // Port

	int bindRes = bind(listenSocket, (sockaddr *)&serverAddr, sizeof(serverAddr));
	if (bindRes == SOCKET_ERROR) {
		printWSErrorAndExit("Couldn't bind socket to the local interface");
	}

	// - Enter in listen mode
	int listenRes = listen(listenSocket, 1);
	if (listenRes == SOCKET_ERROR) {
		printWSErrorAndExit("Couldn't enter listen mode");
	}

	// - Add the listenSocket to the managed list of sockets using addSocket()
	sockets.push_back(listenSocket);

	state = ServerState::Listening;

	LOG("Server set up correctly!");

	return true;
}

bool ModuleNetworkingServer::isRunning() const
{
	return state != ServerState::Stopped;
}



//////////////////////////////////////////////////////////////////////
// Module virtual methods
//////////////////////////////////////////////////////////////////////

bool ModuleNetworkingServer::update()
{
	return true;
}

bool ModuleNetworkingServer::gui()
{
	if (state != ServerState::Stopped)
	{
		// NOTE(jesus): You can put ImGui code here for debugging purposes
		ImGui::Begin("Server Window");

		Texture *tex = App->modResources->server;
		ImVec2 texSize(400.0f, 400.0f * tex->height / tex->width);
		ImGui::Image(tex->shaderResource, texSize);

		ImGui::Text("List of connected users:");

		for (auto &connectedSocket : connectedSockets)
			ImGui::Text("Player name: %s", connectedSocket.playerName.c_str());

		ImGui::Text(chunckOfText.c_str());



		ImGui::End();
	}

	return true;
}



//////////////////////////////////////////////////////////////////////
// ModuleNetworking virtual methods
//////////////////////////////////////////////////////////////////////

bool ModuleNetworkingServer::isListenSocket(SOCKET socket) const
{
	return socket == listenSocket;
}

void ModuleNetworkingServer::onSocketConnected(SOCKET socket, const sockaddr_in &socketAddress)
{
	// Add a new connected socket to the list
	ConnectedSocket connectedSocket;
	connectedSocket.socket = socket;
	connectedSocket.address = socketAddress;
	connectedSockets.push_back(connectedSocket);
}

void ModuleNetworkingServer::onSocketReceivedData(SOCKET socket, const InputMemoryStream& packet)
{
	TextType type;
	packet >> type;

		if (type == TextType::HELP)
		{
			std::string name;
			packet >> name;

			for (auto &connectedSocket : connectedSockets)
			{
				if (connectedSocket.socket == socket)
				{
					connectedSocket.playerName = name;
				}
			}
		}
		else if (type == TextType::MESSAGE)
		{

		}

}

void ModuleNetworkingServer::onSocketDisconnected(SOCKET socket)
{
	// Remove the connected socket from the list
	for (auto it = connectedSockets.begin(); it != connectedSockets.end(); ++it)
	{
		auto &connectedSocket = *it;
		if (connectedSocket.socket == socket)
		{
			connectedSockets.erase(it);
			break;
		}
	}
}

void ModuleNetworkingServer::AddToText(std::string text)
{
}

