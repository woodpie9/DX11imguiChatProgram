#include "ServerProgram.h"
#include "LobbyManager.h"
#include "../WoodnetBase/WoodnetProtocol.h"


ServerProgram::ServerProgram() : m_netID(0), m_socket_cnt(0), m_connection_status(ConnectionStatus::None)
{

}

ServerProgram::~ServerProgram()
{
}

void ServerProgram::init(int listemPort)
{
	// 소켓 객체를 미리 만들어두자. 소켓 핸들이 오픈된 것은 아님
	for (int i = 0; i < MAXIMUM_SOCKET; i++)
	{
		m_socket_ptr_table[i] = new woodnet::TCPSocket;
	}
	m_listen_port = listemPort;

	m_listen_ptr = m_socket_ptr_table[LISTEN_SOCKET_INDEX];

	// 로비 매니저 생성
	this->lobby = new LobbyManager();
	set_m_connection_status(ConnectionStatus::Init);
}

void ServerProgram::clean_up()
{
	for (int i = 0; i < MAXIMUM_SOCKET; i++)
	{
		delete m_socket_ptr_table[i];
	}
	set_m_connection_status(ConnectionStatus::CleanUp);
}

bool ServerProgram::listen()
{
	if (m_listen_ptr == nullptr)
	{
		m_log_msg.emplace_back("pls init first");
		return false;
	}

	if (m_listen_ptr->Open(IPPROTO_TCP) == false)
	{
		m_log_msg.emplace_back("fail open listen socket");
		return false;
	}

	if (m_listen_ptr->EventSelect(FD_ACCEPT | FD_CLOSE) == false)
	{
		m_log_msg.emplace_back("fail set event to listen socket");
		return false;
	}

	SOCKADDR_IN ListenAddr;
	ListenAddr.sin_family = AF_INET;
	InetPtonA(AF_INET, m_listen_ip.c_str(), &ListenAddr.sin_addr);
	ListenAddr.sin_port = htons(m_listen_port);

	if (m_listen_ptr->Bind(ListenAddr) == false)
	{
		m_log_msg.emplace_back("fail bind for listen socket");
		return false;
	}

	if (m_listen_ptr->Listen() == false)
	{
		m_log_msg.emplace_back("fail listen");
		return false;
	}
	m_log_msg.push_back("리슨 시작" + m_listen_ip + ":" + std::to_string(m_listen_port));

	// Generator로 숫자를 증가시키자! 리슨은 0번. 숫자를 리턴하고 갯수를 증가시킨다.
	m_event_table[socket_count_generator()] = m_listen_ptr->GetEventHandle();
	m_listen_ptr->SetNetID(net_id_generator());

	set_m_connection_status(ConnectionStatus::Listen);

	return true;
}

bool ServerProgram::update()
{
	if (m_listen_ptr == nullptr)
	{
		m_log_msg.emplace_back("pls init first..");
		return false;
	}

	if (m_connection_status < ConnectionStatus::Listen)
	{
		m_log_msg.emplace_back("pls Listen first...");
		return false;
	}

	int socketIndex = 0;
	// 시간대기가 WSA_INFINITE 라서 나가는 클라 메시지가 안보내졌다. 시간은 CommonDefines 에서 정의.
	if ((socketIndex = WSAWaitForMultipleEvents(m_socket_cnt, m_event_table, FALSE, DELEY_SERVER_ms, FALSE)) == WSA_WAIT_FAILED)
	{
		m_log_msg.emplace_back("WSAWaitForMultipleEvents() failed");
		return false;
	}
	//	else
	//	CONSOLE_PRINT("WSAWaitForMultipleEvents() is pretty damn OK!\n");
	//	WSAWaitForMultipleEvents 는 어떤 이벤트가 켜졌는지 알려줌.

	//	소켓 객체 배열의 인덱스와 이벤트 배열의 인덱스가 1-1 대응(위치 동일)하도록 만들었다.
	woodnet::TCPSocket* pThisSocket = m_socket_ptr_table[socketIndex];

	// 이벤트가 있을 때
	if (pThisSocket != nullptr)
	{
		//WSAEVENT hEvent = m_EventTable[socketIndex]; 와 pThisSocket->GetEventHandle(); 같은 넘
		//WSAEventSelect 로 소켓과 이벤트 객체를 관련. 구체적으로 어떤 네트워크 이벤트인지 검색
		m_log_msg.push_back("Current Socket is" + std::to_string(static_cast<int>(pThisSocket->GetNetID())));

		WSANETWORKEVENTS NetworkEvents;
		if (WSAEnumNetworkEvents(pThisSocket->GetHandle(), pThisSocket->GetEventHandle(), &NetworkEvents) == SOCKET_ERROR)
		{
			m_log_msg.push_back("WSAEnumNetworkEvents() failed with error" + std::to_string(WSAGetLastError()));
			return false;
		}
		//	else
		//		CONSOLE_PRINT("Server::%d WSAEnumNetworkEvents() should be fine!\n", m_listenPort);

		// 새로운 연결이 발생함
		if (NetworkEvents.lNetworkEvents & FD_ACCEPT)
		{
			if (NetworkEvents.iErrorCode[FD_ACCEPT_BIT] != 0)
			{
				m_log_msg.push_back("WSAEnumNetworkEvents() failed with error" + std::to_string(WSAGetLastError()));
				return false;
			}
			on_net_accept(pThisSocket);
		}

		// HAVE TO DO: 소켓 연결 끊김 처리
		if (NetworkEvents.lNetworkEvents & FD_CLOSE)
		{
			if (NetworkEvents.iErrorCode[FD_CLOSE_BIT] != 0)
			{
				on_net_close(pThisSocket, socketIndex);
				m_log_msg.push_back("FD_CLOSE failed with error" + std::to_string(NetworkEvents.iErrorCode[FD_CLOSE_BIT]));

				return false;
			}
			else
			{
				on_net_close(pThisSocket, socketIndex);
				//m_log_msg.push_back("FD_CLOSE with error" + std::to_string(NetworkEvents.iErrorCode[FD_CLOSE_BIT]()));
				return false;
			}
		}

		if (NetworkEvents.lNetworkEvents & FD_READ)
		{
			if (NetworkEvents.lNetworkEvents & FD_READ && NetworkEvents.iErrorCode[FD_READ_BIT] != 0)
			{
				//m_log_msg.push_back("FD_READ failed with error" + std::to_string(NetworkEvents.iErrorCode[FD_READ_BIT]()));
				return false;
			}

			on_net_receive(pThisSocket);
		}

		if (NetworkEvents.lNetworkEvents & FD_WRITE)
		{
			if (NetworkEvents.lNetworkEvents & FD_WRITE && NetworkEvents.iErrorCode[FD_WRITE_BIT] != 0)
			{
				//m_log_msg.push_back("FD_READ failed with error " + std::to_string(NetworkEvents.iErrorCode[FD_WRITE_BIT]()));
				return false;
			}

			on_net_send(pThisSocket);
		}
	}


	// 모든 소켓들의 큐에 있는 메시지를 보낸다.
	for (int i = 1; i < m_socket_cnt; i++)
	{
		if (m_socket_ptr_table[i]->SendUpdate() == false)
		{
			// 특정 소켓에서 실패함
			m_log_msg.push_back("FD_READ failed with error " + std::to_string(static_cast<int>(m_socket_ptr_table[i]->GetNetID())));
		}
	}

	return true;
}

//bool ServerProgram::close()
//{
//	// close와 clean_up 을 구분을 해야할..까?
//
//	return false;
//}

bool ServerProgram::send(NetworkObjectID NetworkID, void* pPacket, int PacketLen)
{
	// 전체 소켓 갯수 중에서
	for (int i = 1; i < m_socket_cnt; i++)
	{
		// 보내려는 소켓의 ID과 같은 소켓에만 패킷을 Post 한다.
		if (m_socket_ptr_table[i]->GetNetID() == NetworkID)
		{
			return m_socket_ptr_table[i]->PostPacket(static_cast<char*>(pPacket), PacketLen);
		}
	}

	m_log_msg.push_back("NetworkID is Invalid!!! " + std::to_string(static_cast<int>(NetworkID)));

	return false;
}

bool ServerProgram::broadcast(void* pPacket, int PacketLen)
{
	for (int i = 1; i < m_socket_cnt; i++)
	{
		m_socket_ptr_table[i]->PostPacket(static_cast<char*>(pPacket), PacketLen);
	}

	return true;
}

void ServerProgram::on_net_accept(woodnet::TCPSocket* pThisSocket)
{
	m_log_msg.push_back("NetworkID is Invalid!!! " + std::to_string(m_netID));

	//HAVE TO DO: 아래 NULL 이 아니라 접속한 클라의 주소 받아오기 -> 그래야 어디서 접속했는지 안다.
	SOCKET newClientSocket;
	if ((newClientSocket = accept(pThisSocket->GetHandle(), NULL, NULL)) == INVALID_SOCKET)
	{
		m_log_msg.push_back("accept() failed with error " + std::to_string(WSAGetLastError()));
		return;
	}
	else
	{
		m_log_msg.emplace_back("accept() should be OK");
	}


	char ipport[IPPORT_LEN];
	get_client_IPPort(newClientSocket, ipport);
	m_log_msg.push_back("PeerAddr is " + std::to_string(*ipport));


	if (m_socket_cnt >= WSA_MAXIMUM_WAIT_EVENTS)
	{
		m_log_msg.emplace_back("Too many connections - closing socket...");
		closesocket(newClientSocket);
		return;
	}

	woodnet::TCPSocket* pNewSocket = m_socket_ptr_table[socket_count_generator()];
	//pNewSocket 재사용하는 경우를 고려하면 클리어를 마저 구현해야 함
	pNewSocket->Attach(newClientSocket);
	pNewSocket->SetNetID(net_id_generator());


	if (false == pNewSocket->EventSelect(FD_READ | FD_WRITE | FD_CLOSE))
	{
		m_log_msg.emplace_back("EventSelect Fail");
		closesocket(newClientSocket); // 예외 처리 일원화하고 싶어지기 시작함
		return;
	}

	// EventSelect를 해야 이벤트가 생성 됨
	m_event_table[m_socket_cnt - 1] = pNewSocket->GetEventHandle();
	m_log_msg.emplace_back("WSAEventSelect() is OK!");
	m_log_msg.emplace_back("Socket got connected..." + std::to_string((int)pNewSocket->GetNetID()));


	// 로비 매니저에게 정보를 넘겨서 플레이어 객체를 만들고
	this->lobby->new_player(pNewSocket->GetNetID());

	// 패킷을 만들어서 클라에게 접속이 완료되었다고 알린다.
	MSG_S2C_ACCEPT S2CSendPacket;

	S2CSendPacket.packet_size = sizeof(MSG_S2C_ACCEPT);
	S2CSendPacket.packet_id = static_cast<INT16>(COMMON_PAKET_ID::S2C_ACCEPT);
	S2CSendPacket.seqNum = 0;
	S2CSendPacket.NetID = pNewSocket->GetNetID();

	if (send(pNewSocket->GetNetID(), &S2CSendPacket, S2CSendPacket.packet_size) == false)
	{
		m_log_msg.push_back("LobbyServer.Send fail... ID : " + std::to_string(pNewSocket->GetNetID()));
	}
}

void ServerProgram::on_net_close(woodnet::TCPSocket* pThisSocket, int socketIndex)
{
	//HAVE TO DO : 소켓 이벤트로 Close 가 왔을 때. 클라 하나를 닫고 정리한다. 

	// 클라이언트가 나갔음을 전체에게 알린다.
	MSG_S2C_LEAVE_PLAYER S2CBroadCasePacket;

	S2CBroadCasePacket.packet_size = sizeof(MSG_S2C_LEAVE_PLAYER);
	S2CBroadCasePacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::S2C_LEAVE_PLAYER);
	S2CBroadCasePacket.seqNum = 0;
	S2CBroadCasePacket.NetID = pThisSocket->GetNetID();

	std::string str = this->lobby->get_nick_name(pThisSocket->GetNetID());
	const char* cstr = str.c_str();
	char* name = const_cast<char*>(cstr);

	memcpy(S2CBroadCasePacket.NickName, name, MAX_NICKNAME_LEN + 1);

	if (broadcast(&S2CBroadCasePacket, S2CBroadCasePacket.packet_size) == false)
	{
		m_log_msg.push_back("LobbyServer.Broadcast fail ID : " + std::to_string(pThisSocket->GetNetID()));
		return;
	}


	// LobbyManager에서도 지운다.
	// 클라가 정상 종료이면 이미 없어져있겠지만 그래도 한번 더 확인한다.
	this->lobby->delete_player(pThisSocket->GetNetID());

	// 삭제했음을 알린다.
	MSG_S2C_LEAVE_LOBBY_OK S2CSendPacket;

	S2CSendPacket.packet_size = sizeof(MSG_S2C_LEAVE_LOBBY_OK);
	S2CSendPacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::S2C_LEAVE_LOBBY_OK);
	S2CSendPacket.seqNum = 0;

	if (send(pThisSocket->GetNetID(), &S2CSendPacket, S2CSendPacket.packet_size) == false)
	{
		m_log_msg.push_back("LobbyServer.Send fail ID : " + std::to_string(pThisSocket->GetNetID()));
	}


	// SocketCnt바꾸기, SocketPtr 변경. (가장 뒤에 있는 클라를 구멍난 곳으로 옮기기)
	m_log_msg.push_back("Soccket FD_CLOSE! ID : " + std::to_string(pThisSocket->GetNetID()));

	int CloseSocIndex = socketIndex;
	int LastSocNetID = m_socket_cnt - 1;

	// 마지막 소켓 아니면 소켓과 이벤트의 위치를 바꾼다.
	// TODO :: 소켓의 이동이 아니라 삭제된 소켓 번호자리에 새로운 소켓을 넣는 식으로 바꿔볼까?
	if (CloseSocIndex != LastSocNetID)
	{
		// Event Table과 SocketArray의 가장 마지막 유효값과 위치를 바꾼다.
		Swap(m_event_table[CloseSocIndex], m_event_table[LastSocNetID]);
		Swap(m_socket_ptr_table[CloseSocIndex], m_socket_ptr_table[LastSocNetID]);
	}

	/// TODO : Reset 구현
	// 마지막 소켓(삭제할 값)과 인덱스 값을 정리한다.
	m_socket_ptr_table[LastSocNetID]->Close();
	m_socket_ptr_table[LastSocNetID]->Reset();
	m_socket_cnt--;

	/// TODO : NetID는 유일해야 한다.
	/// m_NetID--;
}

void ServerProgram::on_net_receive(woodnet::TCPSocket* pThisSocket)
{
	m_log_msg.emplace_back("FD_READ is OK!");

	// 윈속 버퍼에 들어온 수신된 데이터를 큐에 쌓는다. 그리고 수신 다시 요청
	pThisSocket->RecvUpdate();

	// 수신 큐에 패킷을 가져온다.
	PACKET_HEADER msgHeader;
	while (pThisSocket->PeekPacket(reinterpret_cast<char*>(&msgHeader), sizeof(PACKET_HEADER)))
	{
		// 온전한 패킷이 있다면 패킷을 만들어서 처리.
		memset(&m_packet_rev_buf_, 0, PACKET_SIZE_MAX);

		if (pThisSocket->ReadPacket(m_packet_rev_buf_, msgHeader.packet_size))
		{
			char ipport[IPPORT_LEN];
			get_client_IPPort(pThisSocket->GetHandle(), ipport);
			m_log_msg.push_back("ID : " + std::to_string(pThisSocket->GetNetID()) + " client Seq :: " + std::to_string(msgHeader.seqNum));

			packet_dispatcher(pThisSocket, m_packet_rev_buf_, msgHeader.packet_size);
		}
		else
		{
			break;
		}
	}
}

void ServerProgram::on_net_send(woodnet::TCPSocket* pThisSocket)
{
	m_log_msg.emplace_back("FD_SEND is OK!");
	// 처음 소켓 준비 되면 이벤트 발생., 발생 시점을 확인해 봅시다.
	// 송신: send(buf, size... )  호출하면 윈속 버퍼에 보낼 데이타가 복사된다.
	// 윈속 버퍼에 복사를 못하는 경우 -> 소켓이 유효하지 않음. 
	// 버퍼가 부족.버퍼가 여유가 있으면 다시 신호
}

void ServerProgram::get_client_IPPort(SOCKET ClientSocket, char* ipPort)
{
	/// 접속한 클라이언트 정보를 받아와보자.  함수화 필요
	// 소켓을 인자로 주고... 리턴 값을....  포트를 포함하여 배열에 한번에 붙일까..?
	char addrString[IPPORT_LEN];		// 버퍼의 크기는 IPv4 주소는 16자 이상. IPv6 주소는 46자 이상.
	addrString[0] = 0;

	sockaddr_in m_ipv4Endpoint;
	socklen_t retLength = sizeof(m_ipv4Endpoint);
	if (::getpeername(ClientSocket, (sockaddr*)&m_ipv4Endpoint, &retLength) < 0)
	{
		m_log_msg.emplace_back("getPeerAddr failed");
		return;
	}
	if (retLength > sizeof(m_ipv4Endpoint))
	{
		m_log_msg.emplace_back("getPeerAddr buffer overrun");
		return;
	}

	inet_ntop(AF_INET, &m_ipv4Endpoint.sin_addr, addrString, sizeof(addrString) - 1);

	char str[2] = ":";
	strcat_s(addrString, sizeof(addrString), str);

	// PortNum 번호도 붙이자.
	char PortNum[30];
	PortNum[0] = 0;
	sprintf_s(PortNum, "%d", htons(m_ipv4Endpoint.sin_port));
	strcat_s(addrString, sizeof(addrString), PortNum);

	memcpy(ipPort, addrString, IPPORT_LEN);
}

void ServerProgram::packet_dispatcher(woodnet::TCPSocket* pRecvSocket, void* pPacket, int len)
{
	// 패킷을 보낸 클라이언트의 생존신고

	this->lobby->heart_beat(pRecvSocket->GetNetID());

	auto* pHeader = static_cast<PACKET_HEADER*>(pPacket);

	switch (pHeader->packet_id)
	{
		case static_cast<INT16>(COMMON_PAKET_ID::C2S_HEART_BEAT):
		{
			on_packet_proc_heart_beat(pRecvSocket->GetNetID());
		}
		break;

		case static_cast<INT16>(LOBBY_PACKET_ID::C2S_ENTER_LOBBY):
		{
			on_packet_proc_enter_lobby(pRecvSocket->GetNetID());
		}
		break;

		case static_cast<INT16>(LOBBY_PACKET_ID::C2S_LEAVE_LOBBY):
		{
			on_packet_proc_leave_lobby(pRecvSocket->GetNetID());
		}
		break;

		case static_cast<INT16>(LOBBY_PACKET_ID::C2S_ENTER_CHAT_SER):
		{
			on_packet_proc_enter_chat_ser(pRecvSocket->GetNetID(), pPacket, len);
		}
		break;

		case static_cast<INT16>(LOBBY_PACKET_ID::C2S_CHANGE_NAME):
		{
			on_packet_proc_change_name(pRecvSocket->GetNetID(), pPacket, len);
		}
		break;

		case static_cast<INT16>(LOBBY_PACKET_ID::C2S_CHAT):
		{
			on_packet_proc_chat(pRecvSocket->GetNetID(), pPacket, len);
		}
		break;

		default:
			m_log_msg.push_back("Unknown Packet!" + std::to_string(pHeader->packet_id) + " size : " + std::to_string(len));
	}
}

void ServerProgram::on_packet_proc_heart_beat(NetworkObjectID NetID) const
{
	this->lobby->heart_beat(NetID);
}

void ServerProgram::on_packet_proc_enter_lobby(NetworkObjectID NetId)
{
	// 로비의 플레이어 상태를 변경합니다.
	this->lobby->set_player_state(NetId, LobbyPlayerState::InLobby);

	// 클라에게 보낼 패킷 생성
		// 클라에게 보낼 패킷 생성
	MSG_S2C_ENTER_LOBBY_OK S2CSendPacket;

	S2CSendPacket.packet_size = sizeof(MSG_S2C_ENTER_LOBBY_OK);
	S2CSendPacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::S2C_ENTER_LOBBY_OK);
	S2CSendPacket.seqNum = 0;

	// 보낸다. SendFlush는 Update 가장 하단에 있음.
	if (send(NetId, &S2CSendPacket, S2CSendPacket.packet_size) == false)
	{
		m_log_msg.push_back("LobbyServer.Send fail ID : " + std::to_string(NetId));
		return;
	}
}

void ServerProgram::on_packet_proc_leave_lobby(NetworkObjectID NetId)
{
	// 클라에게 보낼 패킷 생성
	MSG_S2C_LEAVE_LOBBY_OK S2CSendPacket;

	S2CSendPacket.packet_size = sizeof(MSG_S2C_LEAVE_LOBBY_OK);
	S2CSendPacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::S2C_LEAVE_LOBBY_OK);
	S2CSendPacket.seqNum = 0;

	// 보낸다. SendFlush는 Update 가장 하단에 있음.
	if (send(NetId, reinterpret_cast<char*>(&S2CSendPacket), S2CSendPacket.packet_size) == false)
	{
		m_log_msg.push_back("LobbyServer.Send fail ID : " + std::to_string(NetId));
		return;
	}


	// 클라이언트가 나갔음을 전체에게 알린다.
	MSG_S2C_LEAVE_PLAYER S2CBroadCasePacket;

	S2CBroadCasePacket.packet_size = sizeof(MSG_S2C_LEAVE_PLAYER);
	S2CBroadCasePacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::S2C_LEAVE_PLAYER);
	S2CBroadCasePacket.seqNum = 0;
	S2CBroadCasePacket.NetID = NetId;


	std::string str = this->lobby->get_nick_name(NetId);
	const char* cstr = str.c_str();
	char* name = const_cast<char*>(cstr);
	memcpy(S2CBroadCasePacket.NickName, name, MAX_NICKNAME_LEN + 1);

	if (broadcast(reinterpret_cast<char*>(&S2CBroadCasePacket), S2CBroadCasePacket.packet_size) == false)
	{
		m_log_msg.push_back("LobbyServer.Broadcast fail ID : " + std::to_string(NetId));
		return;
	}

	// 로비 메니저에서 삭제한다.
	this->lobby->delete_player(NetId);
}

void ServerProgram::on_packet_proc_enter_chat_ser(NetworkObjectID NetId, void* pC2SPacket, int C2SPacketLen)
{
	MSG_C2S_ENTER_CHAT_SER* C2SRcvPacket = static_cast<MSG_C2S_ENTER_CHAT_SER*>(pC2SPacket);

	MSG_S2C_ENTER_CHAT_SER_OK S2CSendPacket;

	S2CSendPacket.packet_size = sizeof(MSG_S2C_ENTER_CHAT_SER_OK);
	S2CSendPacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::S2C_ENTER_CHAT_SER_OK);
	S2CSendPacket.seqNum = 0;

	char name[MAX_NICKNAME_LEN + 1];
	memcpy(name, C2SRcvPacket->NickName, sizeof(C2SRcvPacket->NickName));//MAX_NICKNAME_LEN + 1);


	if (this->lobby->set_player_nick_name(NetId, name))//name))
	{
		this->lobby->set_player_state(NetId, LobbyPlayerState::InChat);
		S2CSendPacket.Result = true;

		if (send(NetId, &S2CSendPacket, S2CSendPacket.packet_size) == false)
		{
			m_log_msg.push_back("Server.send fail ID : " + std::to_string(NetId));
			return;
		}

		// 새로운 클라이언트가 접속했음을 전체에게 알린다.
		MSG_S2C_NEW_PLAYER S2CBroadCasePacket;

		S2CBroadCasePacket.packet_size = sizeof(MSG_S2C_NEW_PLAYER);
		S2CBroadCasePacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::S2C_NEW_PLAYER);
		S2CBroadCasePacket.seqNum = 0;
		S2CBroadCasePacket.NetID = NetId;

		std::string str = this->lobby->get_nick_name(NetId);
		const char* cstr = str.c_str();
		char* name = const_cast<char*>(cstr);

		memcpy(S2CBroadCasePacket.NickName, name, MAX_NICKNAME_LEN + 1);

		if (broadcast(&S2CBroadCasePacket, S2CBroadCasePacket.packet_size) == false)
		{
			m_log_msg.push_back("Server.Broadcast fail ID : " + std::to_string(NetId));
			return;
		}
	}
	else
	{
		S2CSendPacket.Result = false;

		if (send(NetId, &S2CSendPacket, S2CSendPacket.packet_size) == false)
		{
			m_log_msg.push_back("Server.send fail ID : " + std::to_string(NetId));
			return;
		}
	}
}

void ServerProgram::on_packet_proc_change_name(NetworkObjectID NetId, void* pC2SPacket, int C2SPacketLen)
{
	MSG_C2S_CHANGE_NAME* C2SRcvPacket = static_cast<MSG_C2S_CHANGE_NAME*>(pC2SPacket);
	char name[MAX_NICKNAME_LEN + 1];
	memcpy(name, C2SRcvPacket->NickName, MAX_NICKNAME_LEN + 1);


	if (this->lobby->set_player_nick_name(NetId, name))
	{
		this->lobby->set_player_state(NetId, LobbyPlayerState::InChat);

		// 이름이 변경되었음을 전체에게 알린다.
		MSG_S2C_CHANGE_NAME_OK S2CBroadCastPacket;

		S2CBroadCastPacket.packet_size = sizeof(MSG_S2C_CHANGE_NAME_OK);
		S2CBroadCastPacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::S2C_CHANGE_NAME_OK);
		S2CBroadCastPacket.seqNum = 0;
		S2CBroadCastPacket.NetID = NetId;
		memcpy(S2CBroadCastPacket.NickName, name, MAX_NICKNAME_LEN + 1);

		if (false == broadcast(&S2CBroadCastPacket, S2CBroadCastPacket.packet_size))
		{
			m_log_msg.push_back("server.Broadcastfail ID : " + std::to_string(NetId));
			return;
		}
	}
	else
	{
		/*if (false == LobbyServer.Send(NetId, reinterpret_cast<char*>(&S2CSendPacket), S2CSendPacket.packet_size))
		{
			CONSOLE_PRINT("Server::%d LobbyServer.Send 실패\n", (int)NetId);
			return;
		}*/
	}
}

void ServerProgram::on_packet_proc_chat(NetworkObjectID NetId, void* pC2SPacket, int C2SPacketLen)
{
	MSG_C2S_CHAT* C2SRcvPacket = static_cast<MSG_C2S_CHAT*>(pC2SPacket);

	MSG_S2C_CHAT S2CBroadCasePacket;
	S2CBroadCasePacket.packet_size = sizeof(MSG_S2C_CHAT);
	S2CBroadCasePacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::S2C_CHAT);
	S2CBroadCasePacket.seqNum = 0;
	S2CBroadCasePacket.NetID = NetId;

	std::string str = this->lobby->get_nick_name(NetId);
	const char* cstr = str.c_str();
	char* name = const_cast<char*>(cstr);

	memcpy(S2CBroadCasePacket.NickName, name, MAX_NICKNAME_LEN + 1);

	memcpy(S2CBroadCasePacket.MSG, C2SRcvPacket->MSG, MAX_CHATTING_LEN + 1);

	if (false == broadcast(&S2CBroadCasePacket, S2CBroadCasePacket.packet_size))
	{
		m_log_msg.push_back("server.Broadcast fail ID : " + std::to_string(NetId));
		return;
	}
}

