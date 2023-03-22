#pragma once

#include <string>
#include <vector>

#include "../woodnetBase/WinNetwork.h"
#include "../woodnetBase/TCPSocket.h"
#include "../woodnetBase/WoodnetProtocol.h"
#include "../woodnetBase/CommonDefines.h"
//using namespace woodnet;

// 연결 상태를 제어한다.
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
	// 소켓 통신 관련 제어 함수
	bool init();									// 소켓을 준비한다. (Open)
	bool connect_server(std::string ConnectIP);		// 서버에 연결한다. (Connect)
	bool login_server();							// 로비 서버에 접속상태로 변경
	bool set_nickname(std::string nickname);
	bool change_name(std::string nickname);
	bool network_update();
	//bool clean_up();
	bool close();									// 소켓을 닫는다.
	bool check_connect();

	// 패킷을 만들고, 메시지를 전송한다.
	bool send_chat_msg(std::string msg);
	// 메시지가 담겨있는 벡터를 초기화 하는 함수 (clear)
	void clear_msg_vector() { m_vector_msg.clear(); }

private:
	// 
	// 완성된 패킷을 전송한다.
	bool send(int PacketID, void* pPacket, int PacketLen);
	// 버퍼에 넣어둔 패킷을 송신한다.
	void send_flush();

	// 네트워크 이벤트 처리 관련 함수
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
	// 상태 관리
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

