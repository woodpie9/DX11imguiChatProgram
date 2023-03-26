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

	void init(int listemPort = 5252);	// ������ �����ϰ� ���� ��Ʈ�� �����մϴ�.
	void clean_up();					// ��� ������ �ݳ��մϴ�.

	bool listen();
	bool update();
	//bool close();



	ConnectionStatus get_m_connection_status() const
	{
		return m_connection_status;
	}


private:
	// ��Ŷ ����� ��ȿ�� ���� �ִٰ� �����Ѵ�.
	// Ư�� �������� ��Ŷ�� ������.
	bool send(NetworkObjectID NetworkID, void* pPacket, int PacketLen);
	// ������ ��� �������� ��Ŷ�� ������.
	bool broadcast(void* pPacket, int PacketLen);

	void set_m_connection_status(ConnectionStatus m_connection_status)
	{
		this->m_connection_status = m_connection_status;
	}

	void on_net_accept(woodnet::TCPSocket* pThisSocket);
	void on_net_close(woodnet::TCPSocket* pThisSocket, int socketIndex);
	void on_net_receive(woodnet::TCPSocket* pThisSocket);
	void on_net_send(woodnet::TCPSocket* pThisSocket);

	void get_client_IPPort(SOCKET ClientSocket, char* ipPort);

	NetworkObjectID net_id_generator() { return m_netID++; };
	int socket_count_generator() { return m_socket_cnt++; };

private:
	void packet_dispatcher(woodnet::TCPSocket* pRecvSocket, void* pPacket, int len);

	void on_packet_proc_heart_beat(NetworkObjectID NetID) const;
	void on_packet_proc_enter_lobby(NetworkObjectID NetId);
	void on_packet_proc_leave_lobby(NetworkObjectID NetId);
	void on_packet_proc_enter_chat_ser(NetworkObjectID NetId, void* pC2SPacket, int C2SPacketLen);
	void on_packet_proc_change_name(NetworkObjectID NetId, void* pC2SPacket, int C2SPacketLen);
	void on_packet_proc_chat(NetworkObjectID NetId, void* pC2SPacket, int C2SPacketLen);


private:
	NetworkObjectID m_netID;		// Ŭ���̾�Ʈ�� �����ϴ� ����
	int m_socket_cnt;				// ������ ��ü ����
	WSAEVENT m_event_table[WSA_MAXIMUM_WAIT_EVENTS] = { 0, };
	woodnet::TCPSocket* m_socket_ptr_table[WSA_MAXIMUM_WAIT_EVENTS] = { nullptr, };
	woodnet::TCPSocket* m_listen_ptr = nullptr;

	char m_packet_rev_buf_[PACKET_SIZE_MAX] = { 0, };
	std::string m_listen_ip = "0.0.0.0";
	int m_listen_port;

	// ToDO :: Game Server �� ��Ʈ �и� �ʿ�
	bool m_running = false;

	ConnectionStatus m_connection_status;

	LobbyManager* lobby;

public:
	std::vector<std::string> m_log_msg;
};

template<typename T>
void Swap(T& _input1, T& _input2)
{
	T Temp = _input1;
	_input1 = _input2;
	_input2 = Temp;
}