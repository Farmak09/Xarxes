#include "ModuleNetworkingClient.h"


bool  ModuleNetworkingClient::start(const char * serverAddressStr, int serverPort, const char *pplayerName)
{
	playerName = pplayerName;

	// TODO(jesus): TCP connection stuff
	// - Create the socket
	client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client_socket == INVALID_SOCKET) {
		LOG("Client socket couldn't be creatd");
	}

	// - Create the remote address object
	sockaddr_in serverAddr;
	const int serverAddrLen = sizeof(serverAddr);
	serverAddr.sin_family = AF_INET; // IPv4
	inet_pton(AF_INET, serverAddressStr, &serverAddr.sin_addr);
	serverAddr.sin_port = htons(serverPort); // Port

	// - Connect to the remote address
	int connectRes = connect(client_socket, (const sockaddr*)&serverAddr, serverAddrLen);
	if (connectRes == SOCKET_ERROR) {
		LOG("Couldn't connect to remote address");
	}

	// - Add the created socket to the managed list of sockets using addSocket()
	addSocket(client_socket);

	// If everything was ok... change the state
	state = ClientState::Start;

	return true;
}

bool ModuleNetworkingClient::isRunning() const
{
	return state != ClientState::Stopped;
}

bool ModuleNetworkingClient::update()
{
	if (state == ClientState::Start)
	{
		// TODO(jesus): Send the player name to the server
		int result = send(client_socket, playerName.data(), playerName.size() + 1, 0);
		if (result > 0)
		{
			LOG("%s was sent to server successfully", playerName.data());
			state = ClientState::Logging;
		}
		else
		{
			int err = WSAGetLastError();
			printWSErrorAndContinue("%s couldn't be sent to the server", playerName.data());
		}
	}

	return true;
}

bool ModuleNetworkingClient::gui()
{
	if (state != ClientState::Stopped)
	{
		// NOTE(jesus): You can put ImGui code here for debugging purposes
		ImGui::Begin("Client Window");

		Texture *tex = App->modResources->client;
		ImVec2 texSize(400.0f, 400.0f * tex->height / tex->width);
		ImGui::Image(tex->shaderResource, texSize);

		ImGui::Text("%s connected to the server...", playerName.c_str());

		ImGui::End();
	}

	return true;
}

void ModuleNetworkingClient::onSocketReceivedData(SOCKET socket, byte * data)
{
	state = ClientState::Stopped;
}

void ModuleNetworkingClient::onSocketDisconnected(SOCKET socket)
{
	state = ClientState::Stopped;
}

