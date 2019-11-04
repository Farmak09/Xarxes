#include "ModuleNetworkingClient.h"


bool  ModuleNetworkingClient::start(const char * serverAddressStr, int serverPort, const char *pplayerName)
{
	client.name = pplayerName;

	// TODO(jesus): TCP connection stuff
	// - Create the socket
	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET) {
		printWSErrorAndContinue("Client socket couldn't be creatd");
	}

	// - Create the remote address object
	sockaddr_in serverAddr;
	const int serverAddrLen = sizeof(serverAddr);
	serverAddr.sin_family = AF_INET; // IPv4
	inet_pton(AF_INET, serverAddressStr, &serverAddr.sin_addr);
	serverAddr.sin_port = htons(serverPort); // Port

	// - Connect to the remote address
	int connectRes = connect(clientSocket, (const sockaddr*)&serverAddr, serverAddrLen);
	if (connectRes == SOCKET_ERROR) {
		printWSErrorAndContinue("Couldn't connect to remote address");
	}

	// - Add the created socket to the managed list of sockets using addSocket()
	addSocket(clientSocket);

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
		OutputMemoryStream packet;
		packet << TextType::HELP;
		packet << client.name;

		if (sendPacket(packet, clientSocket))
		{
			state = ClientState::Logging;
		}
		else
		{
			disconnect();
			state = ClientState::Stopped;
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
		if (ImGui::Button("DISCONNECT"))
		{
			disconnect();
			state = ClientState::Stopped;
		}
		ImGui::Text("%s connected to the server...", client.name.c_str());

		ImGui::End();
		ImGui::Begin("Input window");

		if (ImGui::InputText("", inputText, 69, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			DefineTypeAndSendText();
		}


		ImGui::End();
	}

	return true;
}

void ModuleNetworkingClient::onSocketReceivedData(SOCKET socket, const InputMemoryStream& packet)
{
	state = ClientState::Stopped;
}

void ModuleNetworkingClient::onSocketDisconnected(SOCKET socket)
{
	state = ClientState::Stopped;
}

void ModuleNetworkingClient::DefineTypeAndSendText()
{
	CheckTypeOfText();

	SendTextToServer();
}

void ModuleNetworkingClient::CheckTypeOfText()
{
	client.text = inputText;
	client.type = TextType::MESSAGE;
}

void ModuleNetworkingClient::SendTextToServer()
{
	OutputMemoryStream packet;
	packet << client.type;
	packet << client.name;
	packet << client.text;


	if (sendPacket(packet, clientSocket))
	{
		state = ClientState::Logging;
	}
	else
	{
		disconnect();
		state = ClientState::Stopped;
	}
}


