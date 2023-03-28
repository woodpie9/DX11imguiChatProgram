#pragma once
#include <string>
#include <vector>
#include <WS2tcpip.h>

#include "../WoodnetBase/CommonDefines.h"
#include "../WoodnetBase/TCPSocket.h"

class LobbyManager;

//윈속 이벤트 모델에서 배열 최대가 64, 0번 인덱스의 이벤트를 리슨 소켓용으로 사용합니다.
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

	void Init(int listem_port = 5252);	// 소켓을 생성하고 리슨 포트를 지정합니다.
	void CleanUp();					// 모든 소켓을 반납합니다.

	bool Listen();
	bool Update();
	//bool Close();



	ConnectionStatus GetConnectionStatus() const
	{
		return connection_status_;
	}


private:
	// 패킷 헤더에 유효한 값이 있다고 전재한다.
	// 특정 유저에게 패킷을 보낸다.
	bool Send(NetworkObjectID network_id, void* packet, int packet_len);
	// 접속한 모든 유저에게 패킷을 보낸다.
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
	NetworkObjectID net_id_;		// 클라이언트를 구분하는 숫자
	int socket_cnt_;				// 소켓의 전체 개수
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