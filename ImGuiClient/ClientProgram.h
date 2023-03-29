#pragma once

#include <string>
#include <vector>
#include <WS2tcpip.h>

#include "../woodnetBase/TCPSocket.h"

// 연결 상태를 제어한다.
enum class ConnectionStatus : int
{
	None = 0,
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
		return connection_status_;
	}

	void set_connection_status(const ConnectionStatus m_connection_status)
	{
		this->connection_status_ = m_connection_status;
	}

	_Error get_last_error() const
	{
		return last_error_;
	}

	void set_last_error(const _Error m_last_error)
	{
		last_error_ = m_last_error;
	}

public:
	// 소켓 통신 관련 제어 함수
	bool Init();									// 소켓을 준비한다. (Open)
	bool ConnectServer(std::string connect_ip);		// 서버에 연결한다. (Connect)
	bool LoginServer();							// 로비 서버에 접속상태로 변경
	bool SetNickname(std::string nickname);
	bool ChangeName(std::string nickname);
	bool NetworkUpdate();
	//bool clean_up();
	bool Close();									// 소켓을 닫는다.
	bool CheckConnect();

	// 패킷을 만들고, 메시지를 전송한다.
	bool SendChatMsg(std::string msg);
	// 메시지가 담겨있는 벡터를 초기화 하는 함수 (clear)
	void clear_msg_vector() { vector_msg_.clear(); }

private:
	// 
	// 완성된 패킷을 전송한다.
	bool Send(int packet_id, void* p_packet, int packet_len);
	// 버퍼에 넣어둔 패킷을 송신한다.
	void SendFlush();

	// 네트워크 이벤트 처리 관련 함수
	bool OnNetConnect(woodnet::TCPSocket* p_this_socket);
	bool OnNetClose(woodnet::TCPSocket* p_this_socket);
	bool OnNetRecv(woodnet::TCPSocket* p_this_socket);
	bool OnNetSend(woodnet::TCPSocket* pThisSocket);

private:
	bool PacketDispatcher(woodnet::TCPSocket* recv_sock, void* p_packet, int len);
	bool OnPacketProcAccept(woodnet::TCPSocket* recv_sock, void* ps2c_packet, int c2s_packet_len);
	bool OnPacketProcEnterLobby(woodnet::TCPSocket* recv_sock, void* ps2c_packet, int c2s_packet_len);
	bool OnPacketProcEnterChat(woodnet::TCPSocket* recv_sock, void* ps2c_packet, int c2s_packet_len);
	bool OnPacketProcNewPlayer(woodnet::TCPSocket* recv_sock, void* ps2c_packet, int c2s_packet_len);
	bool OnPacketProcLeavePlayer(woodnet::TCPSocket* recv_sock, void* ps2c_packet, int c2s_packet_len);
	bool OnPacketProcChangeName(woodnet::TCPSocket* recv_sock, void* ps2c_packet, int c2s_packet_len);
	bool OnPacketProcChat(woodnet::TCPSocket* recv_sock, void* ps2c_packet, int c2s_packet_len);
	bool OnPacketProcNotice(woodnet::TCPSocket* recv_sock, void* ps2c_packet, int c2s_packet_len);

private:
	// 상태 관리
	ConnectionStatus connection_status_ = ConnectionStatus::None;
	_Error last_error_ = _Error::not_init;

private:
	WSAEVENT event_;
	woodnet::TCPSocket connector_;
	int port_ = 5252;
	int my_net_id_;
	
	char packet_recv_buf_[PACKET_SIZE_MAX] = { 0, };


	bool client_;
	bool connect_;

public:
	std::vector<std::string> vector_msg_;
	std::vector<std::string> log_msg_;

};