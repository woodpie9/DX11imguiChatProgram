#pragma once
#include <string>
#include <vector>
#include <WS2tcpip.h>

#include "../WoodnetBase/CommonDefines.h"
#include "../WoodnetBase/TCPSocket.h"

class LobbyManager;

//���� �̺�Ʈ �𵨿��� �迭 �ִ밡 64, 0�� �ε����� �̺�Ʈ�� ���� ���Ͽ����� ����մϴ�.
#define MAXIMUM_SOCKET WSA_MAXIMUM_WAIT_EVENTS 

enum class ConnectionStatus : int
{
	None = 0,
	CleanUp,
	Disconnected,
	Init,
	Listen,
	Connecting,
	Connected,
};

class ServerProgram
{
public:
	ServerProgram();
	~ServerProgram();

	void Init(int listem_port = 5252);	// ������ �����ϰ� ���� ��Ʈ�� �����մϴ�.
	void CleanUp();					// ��� ������ �ݳ��մϴ�.

	bool Listen();
	bool Update();
	//bool Close();



	ConnectionStatus GetConnectionStatus() const
	{
		return connection_status_;
	}


private:
	// ��Ŷ ����� ��ȿ�� ���� �ִٰ� �����Ѵ�.
	// Ư�� �������� ��Ŷ�� ������.
	bool Send(NetworkObjectID network_id, void* packet, int packet_len);
	// ������ ��� �������� ��Ŷ�� ������.
	bool Broadcast(void* packet, int packet_len);

	void SetConnectionStatus(ConnectionStatus connection_status)
	{
		this->connection_status_ = connection_status;
	}

	void OnNetAccept(woodnet::TCPSocket* this_socket);
	void OnNetClose(woodnet::TCPSocket* this_socket, int socket_index);
	void OnNetReceive(woodnet::TCPSocket* this_socket);
	void OnNetSend(woodnet::TCPSocket* this_socket);

	void GetClientIpPort(SOCKET client_socket, char* ip_port);

	NetworkObjectID NetIdGenerator() { return net_id_++; };
	int SocketCountGenerator() { return socket_cnt_++; };

private:
	void PacketDispatcher(woodnet::TCPSocket* recv_socket, void* packet, int len);

	void OnPacketProcessHeartBeat(NetworkObjectID net_id) const;
	void OnPacketProcessEnterLobby(NetworkObjectID net_id);
	void OnPacketProcessLeaveLobby(NetworkObjectID net_id);
	void OnPacketProcessEnterChatSer(NetworkObjectID net_id, void* C2S_packet, int C2S_packet_len);
	void OnPacketProcessChangeName(NetworkObjectID net_id, void* C2S_packet, int C2S_packet_len);
	void OnPacketProcessChat(NetworkObjectID net_id, void* C2S_packet, int C2S_packet_len);


private:
	NetworkObjectID net_id_;		// Ŭ���̾�Ʈ�� �����ϴ� ����
	int socket_cnt_;				// ������ ��ü ����
	WSAEVENT event_table_[WSA_MAXIMUM_WAIT_EVENTS] = { 0, };
	woodnet::TCPSocket* socket_ptr_table_[WSA_MAXIMUM_WAIT_EVENTS] = { nullptr, };
	woodnet::TCPSocket* listen_ptr_ = nullptr;

	char packet_rev_buf_[PACKET_SIZE_MAX] = { 0, };
	std::string listen_ip_ = "0.0.0.0";
	int listen_port_;

	ConnectionStatus connection_status_;

	LobbyManager* lobby_;

public:
	std::vector<std::string> log_msg_;
};

template<typename T>
void Swap(T& _input1, T& _input2)
{
	T Temp = _input1;
	_input1 = _input2;
	_input2 = Temp;
}