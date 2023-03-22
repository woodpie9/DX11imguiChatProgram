#pragma once

#include <string>
#include <vector>

#include "../woodnetBase/WinNetwork.h"
#include "../woodnetBase/TCPSocket.h"
#include "../woodnetBase/WoodnetProtocol.h"
#include "../woodnetBase/CommonDefines.h"
//using namespace woodnet;

// ���� ���¸� �����Ѵ�.
enum class ConnectionStatus
{
	None,
	Oppend,
	SetEvent,
	Connecting,
	Connected,
	OnChat,
	Disconnected,
	Closed
};

enum class _Error : int
{
	fatal = -1,
	success = 0,
	error,
	not_init,
	already_init,
	not_connect,
	already_connect,
};

class ClientProgram
{
public:
	ClientProgram();
	~ClientProgram() {};

	ConnectionStatus get_connection_status() const
	{
		return m_connection_status;
	}

	void set_connection_status(const ConnectionStatus m_connection_status)
	{
		this->m_connection_status = m_connection_status;
	}

	_Error get_last_error() const
	{
		return m_last_error_;
	}

	void set_last_error(const _Error m_last_error)
	{
		m_last_error_ = m_last_error;
	}

public:
	// ���� ��� ���� ���� �Լ�
	bool init();									// ������ �غ��Ѵ�. (Open)
	bool connect_server(std::string ConnectIP);		// ������ �����Ѵ�. (Connect)
	bool login_server();							// �κ� ������ ���ӻ��·� ����
	bool set_nickname(std::string nickname);
	bool change_name(std::string nickname);
	bool network_update();
	//bool clean_up();
	bool close();									// ������ �ݴ´�.
	bool check_connect();

	// ��Ŷ�� �����, �޽����� �����Ѵ�.
	bool send_chat_msg(std::string msg);
	// �޽����� ����ִ� ���͸� �ʱ�ȭ �ϴ� �Լ� (clear)
	void clear_msg_vector() { m_vector_msg.clear(); }

private:
	// 
	// �ϼ��� ��Ŷ�� �����Ѵ�.
	bool send(int PacketID, void* pPacket, int PacketLen);
	// ���ۿ� �־�� ��Ŷ�� �۽��Ѵ�.
	void send_flush();

	// ��Ʈ��ũ �̺�Ʈ ó�� ���� �Լ�
	bool on_net_connect(woodnet::TCPSocket* pThisSocket);
	bool on_net_close(woodnet::TCPSocket* pThisSocket);
	bool on_net_recv(woodnet::TCPSocket* pThisSocket);
	bool on_net_send(woodnet::TCPSocket* pThisSocket);

private:
	bool PacketDispatcher(woodnet::TCPSocket* RecvSock, void* pPacket, int len);
	bool on_packet_proc_accept(woodnet::TCPSocket* RecvSock, void* pS2CPacket, int C2SPacketLen);
	bool on_packet_proc_enter_lobby(woodnet::TCPSocket* RecvSock, void* pS2CPacket, int C2SPacketLen);
	bool on_packet_proc_enter_chat(woodnet::TCPSocket* RecvSock, void* pS2CPacket, int C2SPacketLen);
	bool on_packet_proc_new_player(woodnet::TCPSocket* RecvSock, void* pS2CPacket, int C2SPacketLen);
	bool on_packet_proc_leave_player(woodnet::TCPSocket* RecvSock, void* pS2CPacket, int C2SPacketLen);
	bool on_packet_proc_change_name(woodnet::TCPSocket* RecvSock, void* pS2CPacket, int C2SPacketLen);
	bool on_packet_proc_chat(woodnet::TCPSocket* RecvSock, void* pS2CPacket, int C2SPacketLen);
	bool on_packet_proc_notice(woodnet::TCPSocket* RecvSock, void* pS2CPacket, int C2SPacketLen);

private:
	// ���� ����
	ConnectionStatus m_connection_status = ConnectionStatus::None;
	_Error m_last_error_ = _Error::not_init;

private:
	WSAEVENT m_Event;
	woodnet::TCPSocket m_Connector;
	int m_port = 5252;
	int MyNetID;

	static const int PACKET_SIZE_MAX = 1024;
	char m_PacketRecvBuf[PACKET_SIZE_MAX] = { 0, };


	bool OnClient;
	bool OnConnect;

	std::vector<std::string> m_vector_msg;


};

