#include "ServerProgram.h"
#include "LobbyManager.h"
#include "../WoodnetBase/WoodnetProtocol.h"


ServerProgram::ServerProgram() : net_id_(0), socket_cnt_(0), connection_status_(ConnectionStatus::None)
{

}

ServerProgram::~ServerProgram()
{
}

void ServerProgram::Init(int listem_port)
{
	// 소켓 객체를 미리 만들어두자. 소켓 핸들이 오픈된 것은 아님
	for (int i = 0; i < MAXIMUM_SOCKET; i++)
	{
		socket_ptr_table_[i] = new woodnet::TCPSocket;
	}
	listen_port_ = listem_port;

	listen_ptr_ = socket_ptr_table_[LISTEN_SOCKET_INDEX];

	// 로비 매니저 생성
	this->lobby_ = new LobbyManager();
	SetConnectionStatus(ConnectionStatus::Init);
	
	log_msg_.push_back("init :" + listen_ip_ + ":" + std::to_string(listen_port_));
}

void ServerProgram::CleanUp()
{
	for (int i = 0; i < MAXIMUM_SOCKET; i++)
	{
		delete socket_ptr_table_[i];
	}
	SetConnectionStatus(ConnectionStatus::CleanUp);
}

bool ServerProgram::Listen()
{
	if (listen_ptr_ == nullptr)
	{
		log_msg_.emplace_back("pls init first");
		return false;
	}

	if (listen_ptr_->Open(IPPROTO_TCP) == false)
	{
		log_msg_.emplace_back("fail open listen socket");
		return false;
	}

	if (listen_ptr_->EventSelect(FD_ACCEPT | FD_CLOSE) == false)
	{
		log_msg_.emplace_back("fail set event to listen socket");
		return false;
	}

	SOCKADDR_IN listen_addr;
	listen_addr.sin_family = AF_INET;
	InetPtonA(AF_INET, listen_ip_.c_str(), &listen_addr.sin_addr);
	listen_addr.sin_port = htons(listen_port_);

	if (listen_ptr_->Bind(listen_addr) == false)
	{
		log_msg_.emplace_back("fail bind for listen socket");
		return false;
	}

	if (listen_ptr_->Listen() == false)
	{
		log_msg_.emplace_back("fail listen");
		return false;
	}

	log_msg_.push_back("listen start :" + listen_ip_ + ":" + std::to_string(listen_port_));

	// Generator로 숫자를 증가시키자! 리슨은 0번. 숫자를 리턴하고 갯수를 증가시킨다.
	event_table_[SocketCountGenerator()] = listen_ptr_->GetEventHandle();
	listen_ptr_->SetNetId(NetIdGenerator());

	SetConnectionStatus(ConnectionStatus::Listen);

	return true;
}

bool ServerProgram::Update()
{
	if (listen_ptr_ == nullptr)
	{
		log_msg_.emplace_back("pls init first..");
		return false;
	}

	if (connection_status_ < ConnectionStatus::Listen)
	{
		log_msg_.emplace_back("pls Listen first...");
		return false;
	}

	int socket_index = 0;
	// 시간대기가 WSA_INFINITE 라서 나가는 클라 메시지가 안보내졌다. 시간은 CommonDefines 에서 정의.
	if ((socket_index = WSAWaitForMultipleEvents(socket_cnt_, event_table_, FALSE, DELEY_SERVER_MS, FALSE)) == WSA_WAIT_FAILED)
	{
		log_msg_.emplace_back("WSAWaitForMultipleEvents() failed");
		return false;
	}

	// TODO : Event의 리턴을 int 로 받는것이 아닌 정식으로 리턴 받아서 예외처리를 해야 한다
	if (socket_index == WSA_WAIT_TIMEOUT) // 258 이면...
	{
		// index가 258일때 pthisSocket이 null이 안뜨는 경우가 발생함... 뭐냐?
		// 임시 처리
		return true;
	}

	//	else
	//	CONSOLE_PRINT("WSAWaitForMultipleEvents() is pretty damn OK!\n");
	//	WSAWaitForMultipleEvents 는 어디서(어느 소켓에서) 어떤 이벤트가 켜졌는지 알려줌.

	//	소켓 객체 배열의 인덱스와 이벤트 배열의 인덱스가 1-1 대응(위치 동일)하도록 만들었다.
	woodnet::TCPSocket* this_socket = socket_ptr_table_[socket_index];

	// 이벤트가 있을 때
	if (this_socket != nullptr)
	{
		//WSAEVENT hEvent = m_EventTable[socketIndex]; 와 pThisSocket->GetEventHandle(); 같은 넘
		//WSAEventSelect 로 소켓과 이벤트 객체를 관련. 구체적으로 어떤 네트워크 이벤트인지 검색
		log_msg_.push_back("Current Socket is" 
			+ std::to_string(this_socket->GetNetId()));

		WSANETWORKEVENTS network_events;
		if (WSAEnumNetworkEvents(this_socket->GetHandle(), this_socket->GetEventHandle(),
			&network_events) == SOCKET_ERROR)
		{
			log_msg_.push_back("1 WSAEnumNetworkEvents() failed with error"
				+ std::to_string(WSAGetLastError()));
			return false;
		}
		//	else
		//	CONSOLE_PRINT("Server::%d WSAEnumNetworkEvents() should be fine!\n", m_listenPort);

		// 새로운 연결이 발생함
		if (network_events.lNetworkEvents & FD_ACCEPT)
		{
			if (network_events.iErrorCode[FD_ACCEPT_BIT] != 0)
			{
				log_msg_.push_back("2 WSAEnumNetworkEvents() failed with error" 
					+ std::to_string(WSAGetLastError()));
				return false;
			}
			OnNetAccept(this_socket);
		}

		// HAVE TO DO: 소켓 연결 끊김 처리
		if (network_events.lNetworkEvents & FD_CLOSE)
		{
			if (network_events.iErrorCode[FD_CLOSE_BIT] != 0)
			{
				OnNetClose(this_socket, socket_index);
				log_msg_.push_back("FD_CLOSE failed with error" 
					+ std::to_string(network_events.iErrorCode[FD_CLOSE_BIT]));

				return false;
			}
			else
			{
				OnNetClose(this_socket, socket_index);
				//log_msg.push_back("FD_CLOSE with error"
				//		+ std::to_string(NetworkEvents.iErrorCode[FD_CLOSE_BIT]()));
				return false;
			}
		}

		if (network_events.lNetworkEvents & FD_READ)
		{
			if (network_events.lNetworkEvents & FD_READ 
				&& network_events.iErrorCode[FD_READ_BIT] != 0)
			{
				//log_msg.push_back("FD_READ failed with error"
				//	+ std::to_string(NetworkEvents.iErrorCode[FD_READ_BIT]()));
				return false;
			}

			OnNetReceive(this_socket);
		}

		if (network_events.lNetworkEvents & FD_WRITE)
		{
			if (network_events.lNetworkEvents & FD_WRITE 
				&& network_events.iErrorCode[FD_WRITE_BIT] != 0)
			{
				//log_msg.push_back("FD_READ failed with error "
				//	+ std::to_string(NetworkEvents.iErrorCode[FD_WRITE_BIT]()));
				return false;
			}

			OnNetSend(this_socket);
		}
	}


	// 모든 소켓들의 큐에 있는 메시지를 보낸다.
	for (int i = 1; i < socket_cnt_; i++)
	{
		if (socket_ptr_table_[i]->SendUpdate() == false)
		{
			// 특정 소켓에서 실패함
			log_msg_.push_back("FD_READ failed with error " 
				+ std::to_string(static_cast<int>(socket_ptr_table_[i]->GetNetId())));
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

bool ServerProgram::Send(NetworkObjectID NetworkID, void* pPacket, int PacketLen)
{
	// 전체 소켓 갯수 중에서
	for (int i = 1; i < socket_cnt_; i++)
	{
		// 보내려는 소켓의 ID과 같은 소켓에만 패킷을 Post 한다.
		if (socket_ptr_table_[i]->GetNetId() == NetworkID)
		{
			return socket_ptr_table_[i]->PostPacket(static_cast<char*>(pPacket), PacketLen);
		}
	}

	log_msg_.push_back("NetworkID is Invalid!!! " + std::to_string(static_cast<int>(NetworkID)));

	return false;
}

bool ServerProgram::Broadcast(void* packet, int packet_len)
{
	for (int i = 1; i < socket_cnt_; i++)
	{
		socket_ptr_table_[i]->PostPacket(static_cast<char*>(packet), packet_len);
	}

	return true;
}

void ServerProgram::OnNetAccept(woodnet::TCPSocket* this_socket)
{
	log_msg_.push_back("NetworkID is Invalid!!! " 
		+ std::to_string(net_id_));

	//HAVE TO DO: 아래 NULL 이 아니라 접속한 클라의 주소 받아오기 -> 그래야 어디서 접속했는지 안다.
	SOCKET new_client_socket;
	if ((new_client_socket = accept(this_socket->GetHandle(), NULL, NULL)) 
		== INVALID_SOCKET)
	{
		log_msg_.push_back("accept() failed with error " 
			+ std::to_string(WSAGetLastError()));
		return;
	}
	else
	{
		log_msg_.emplace_back("accept() should be OK");
	}


	char ip_port[IPPORT_LEN];
	GetClientIpPort(new_client_socket, ip_port);
	log_msg_.push_back("PeerAddr is " + std::to_string(*ip_port));


	if (socket_cnt_ >= WSA_MAXIMUM_WAIT_EVENTS)
	{
		log_msg_.emplace_back("Too many connections - closing socket...");
		closesocket(new_client_socket);
		return;
	}

	woodnet::TCPSocket* new_socket = socket_ptr_table_[SocketCountGenerator()];
	//pNewSocket 재사용하는 경우를 고려하면 클리어를 마저 구현해야 함
	new_socket->Attach(new_client_socket);
	new_socket->SetNetId(NetIdGenerator());


	if (false == new_socket->EventSelect(FD_READ | FD_WRITE | FD_CLOSE))
	{
		log_msg_.emplace_back("EventSelect Fail");
		closesocket(new_client_socket); // 예외 처리 일원화하고 싶어지기 시작함
		return;
	}

	// EventSelect를 해야 이벤트가 생성 됨
	event_table_[socket_cnt_ - 1] = new_socket->GetEventHandle();
	log_msg_.emplace_back("WSAEventSelect() is OK!");
	log_msg_.emplace_back("Socket got connected..." 
		+ std::to_string((int)new_socket->GetNetId()));


	// 로비 매니저에게 정보를 넘겨서 플레이어 객체를 만들고
	this->lobby_->NewPlayer(new_socket->GetNetId());

	// 패킷을 만들어서 클라에게 접속이 완료되었다고 알린다.
	MSG_S2C_ACCEPT S2CSendPacket;

	S2CSendPacket.packet_size = sizeof(MSG_S2C_ACCEPT);
	S2CSendPacket.packet_id = static_cast<INT16>(COMMON_PAKET_ID::S2C_ACCEPT);
	S2CSendPacket.seqNum = 0;
	S2CSendPacket.NetID = new_socket->GetNetId();

	if (Send(new_socket->GetNetId(), &S2CSendPacket, S2CSendPacket.packet_size) == false)
	{
		log_msg_.push_back("LobbyServer.Send fail... ID : " 
			+ std::to_string(new_socket->GetNetId()));
	}
}

void ServerProgram::OnNetClose(woodnet::TCPSocket* this_socket, int socket_index)
{
	//HAVE TO DO : 소켓 이벤트로 Close 가 왔을 때. 클라 하나를 닫고 정리한다. 

	// 클라이언트가 나갔음을 전체에게 알린다.
	MSG_S2C_LEAVE_PLAYER S2CBroadCasePacket;

	S2CBroadCasePacket.packet_size = sizeof(MSG_S2C_LEAVE_PLAYER);
	S2CBroadCasePacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::S2C_LEAVE_PLAYER);
	S2CBroadCasePacket.seqNum = 0;
	S2CBroadCasePacket.NetID = this_socket->GetNetId();

	std::string str = this->lobby_->GetNickName(this_socket->GetNetId());
	const char* cstr = str.c_str();
	char* name = const_cast<char*>(cstr);

	memcpy(S2CBroadCasePacket.NickName, name, MAX_NICKNAME_LEN + 1);

	if (Broadcast(&S2CBroadCasePacket, S2CBroadCasePacket.packet_size) == false)
	{
		log_msg_.push_back("LobbyServer.Broadcast fail ID : " 
			+ std::to_string(this_socket->GetNetId()));
		return;
	}


	// LobbyManager에서도 지운다.
	// 클라가 정상 종료이면 이미 없어져있겠지만 그래도 한번 더 확인한다.
	this->lobby_->DeletePlayer(this_socket->GetNetId());

	// 삭제했음을 알린다.
	MSG_S2C_LEAVE_LOBBY_OK S2CSendPacket;

	S2CSendPacket.packet_size = sizeof(MSG_S2C_LEAVE_LOBBY_OK);
	S2CSendPacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::S2C_LEAVE_LOBBY_OK);
	S2CSendPacket.seqNum = 0;

	if (Send(this_socket->GetNetId(), &S2CSendPacket, S2CSendPacket.packet_size) == false)
	{
		log_msg_.push_back("LobbyServer.Send fail ID : " 
			+ std::to_string(this_socket->GetNetId()));
	}


	// SocketCnt바꾸기, SocketPtr 변경. (가장 뒤에 있는 클라를 구멍난 곳으로 옮기기)
	log_msg_.push_back("Soccket FD_CLOSE! ID : " 
		+ std::to_string(this_socket->GetNetId()));

	int CloseSocIndex = socket_index;
	int LastSocNetID = socket_cnt_ - 1;

	// 마지막 소켓 아니면 소켓과 이벤트의 위치를 바꾼다.
	// TODO :: 소켓의 이동이 아니라 삭제된 소켓 번호자리에 새로운 소켓을 넣는 식으로 바꿔볼까?
	if (CloseSocIndex != LastSocNetID)
	{
		// Event Table과 SocketArray의 가장 마지막 유효값과 위치를 바꾼다.
		Swap(event_table_[CloseSocIndex], event_table_[LastSocNetID]);
		Swap(socket_ptr_table_[CloseSocIndex], socket_ptr_table_[LastSocNetID]);
	}

	/// TODO : Reset 구현
	// 마지막 소켓(삭제할 값)과 인덱스 값을 정리한다.
	socket_ptr_table_[LastSocNetID]->Close();
	socket_ptr_table_[LastSocNetID]->Reset();
	socket_cnt_--;

	/// TODO : NetID는 유일해야 한다.
	/// m_NetID--;
}

void ServerProgram::OnNetReceive(woodnet::TCPSocket* this_socket)
{
	log_msg_.emplace_back("FD_READ is OK!");

	// 윈속 버퍼에 들어온 수신된 데이터를 큐에 쌓는다. 그리고 수신 다시 요청
	this_socket->RecvUpdate();

	// 수신 큐에 패킷을 가져온다.
	PACKET_HEADER msgHeader;
	while (this_socket->PeekPacket(reinterpret_cast<char*>(&msgHeader), sizeof(PACKET_HEADER)))
	{
		// 온전한 패킷이 있다면 패킷을 만들어서 처리.
		memset(&packet_rev_buf_, 0, PACKET_SIZE_MAX);

		if (this_socket->ReadPacket(packet_rev_buf_, msgHeader.packet_size))
		{
			char ipport[IPPORT_LEN];
			GetClientIpPort(this_socket->GetHandle(), ipport);
			log_msg_.push_back("ID : " + std::to_string(this_socket->GetNetId()) 
				+ " client Seq :: " + std::to_string(msgHeader.seqNum));

			PacketDispatcher(this_socket, packet_rev_buf_, msgHeader.packet_size);
		}
		else
		{
			break;
		}
	}
}

void ServerProgram::OnNetSend(woodnet::TCPSocket* this_socket)
{
	log_msg_.emplace_back("FD_SEND is OK!");
	// 처음 소켓 준비 되면 이벤트 발생., 발생 시점을 확인해 봅시다.
	// 송신: send(buf, size... )  호출하면 윈속 버퍼에 보낼 데이타가 복사된다.
	// 윈속 버퍼에 복사를 못하는 경우 -> 소켓이 유효하지 않음. 
	// 버퍼가 부족.버퍼가 여유가 있으면 다시 신호
}

void ServerProgram::GetClientIpPort(SOCKET client_socket, char* ip_port)
{
	/// 접속한 클라이언트 정보를 받아와보자.  함수화 필요
	// 소켓을 인자로 주고... 리턴 값을....  포트를 포함하여 배열에 한번에 붙일까..?
	char addrString[IPPORT_LEN];		// 버퍼의 크기는 IPv4 주소는 16자 이상. IPv6 주소는 46자 이상.
	addrString[0] = 0;

	sockaddr_in m_ipv4Endpoint;
	socklen_t retLength = sizeof(m_ipv4Endpoint);
	if (::getpeername(client_socket, (sockaddr*)&m_ipv4Endpoint, &retLength) < 0)
	{
		log_msg_.emplace_back("getPeerAddr failed");
		return;
	}
	if (retLength > sizeof(m_ipv4Endpoint))
	{
		log_msg_.emplace_back("getPeerAddr buffer overrun");
		return;
	}

	inet_ntop(AF_INET, &m_ipv4Endpoint.sin_addr, addrString, sizeof(addrString) - 1);

	const char str[2] = ":";
	strcat_s(addrString, sizeof(addrString), str);

	// PortNum 번호도 붙이자.
	char PortNum[30];
	PortNum[0] = 0;
	sprintf_s(PortNum, "%d", htons(m_ipv4Endpoint.sin_port));
	strcat_s(addrString, sizeof(addrString), PortNum);

	memcpy(ip_port, addrString, IPPORT_LEN);
}

void ServerProgram::PacketDispatcher(woodnet::TCPSocket* recv_socket, void* packet, int len)
{
	// 패킷을 보낸 클라이언트의 생존신고

	this->lobby_->HeartBeat(recv_socket->GetNetId());

	auto* packet_header = static_cast<PACKET_HEADER*>(packet);

	switch (packet_header->packet_id)
	{
		case static_cast<INT16>(COMMON_PAKET_ID::C2S_HEART_BEAT):
		{
			OnPacketProcessHeartBeat(recv_socket->GetNetId());
		}
		break;

		case static_cast<INT16>(LOBBY_PACKET_ID::C2S_ENTER_LOBBY):
		{
			OnPacketProcessEnterLobby(recv_socket->GetNetId());
		}
		break;

		case static_cast<INT16>(LOBBY_PACKET_ID::C2S_LEAVE_LOBBY):
		{
			OnPacketProcessLeaveLobby(recv_socket->GetNetId());
		}
		break;

		case static_cast<INT16>(LOBBY_PACKET_ID::C2S_ENTER_CHAT_SER):
		{
			OnPacketProcessEnterChatSer(recv_socket->GetNetId(), packet, len);
		}
		break;

		case static_cast<INT16>(LOBBY_PACKET_ID::C2S_CHANGE_NAME):
		{
			OnPacketProcessChangeName(recv_socket->GetNetId(), packet, len);
		}
		break;

		case static_cast<INT16>(LOBBY_PACKET_ID::C2S_CHAT):
		{
			OnPacketProcessChat(recv_socket->GetNetId(), packet, len);
		}
		break;

		default:
			log_msg_.push_back("Unknown Packet!" + std::to_string(packet_header->packet_id) + " size : " + std::to_string(len));
	}
}

void ServerProgram::OnPacketProcessHeartBeat(NetworkObjectID net_id) const
{
	this->lobby_->HeartBeat(net_id);
}

void ServerProgram::OnPacketProcessEnterLobby(NetworkObjectID net_id)
{
	// 로비의 플레이어 상태를 변경합니다.
	this->lobby_->SetPlayerState(net_id, LobbyPlayerState::InLobby);

	// 클라에게 보낼 패킷 생성
	MSG_S2C_ENTER_LOBBY_OK S2CSendPacket;

	S2CSendPacket.packet_size = sizeof(MSG_S2C_ENTER_LOBBY_OK);
	S2CSendPacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::S2C_ENTER_LOBBY_OK);
	S2CSendPacket.seqNum = 0;

	// 보낸다. SendFlush는 Update 가장 하단에 있음.
	if (Send(net_id, &S2CSendPacket, S2CSendPacket.packet_size) == false)
	{
		log_msg_.push_back("Enter Lobby Send fail ID : " + std::to_string(net_id));
		return;
	}
}

void ServerProgram::OnPacketProcessLeaveLobby(NetworkObjectID net_id)
{
	// 클라에게 보낼 패킷 생성
	MSG_S2C_LEAVE_LOBBY_OK S2CSendPacket;

	S2CSendPacket.packet_size = sizeof(MSG_S2C_LEAVE_LOBBY_OK);
	S2CSendPacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::S2C_LEAVE_LOBBY_OK);
	S2CSendPacket.seqNum = 0;

	// 보낸다. SendFlush는 Update 가장 하단에 있음.
	if (Send(net_id, reinterpret_cast<char*>(&S2CSendPacket), S2CSendPacket.packet_size) == false)
	{
		log_msg_.push_back("Leave Lobby Send fail ID : " + std::to_string(net_id));
		return;
	}


	// 클라이언트가 나갔음을 전체에게 알린다.
	MSG_S2C_LEAVE_PLAYER S2CBroadCasePacket;

	S2CBroadCasePacket.packet_size = sizeof(MSG_S2C_LEAVE_PLAYER);
	S2CBroadCasePacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::S2C_LEAVE_PLAYER);
	S2CBroadCasePacket.seqNum = 0;
	S2CBroadCasePacket.NetID = net_id;


	std::string string = this->lobby_->GetNickName(net_id);
	const char* cstr = string.c_str();
	char* name = const_cast<char*>(cstr);
	memcpy(S2CBroadCasePacket.NickName, name, MAX_NICKNAME_LEN + 1);

	if (Broadcast(&S2CBroadCasePacket, S2CBroadCasePacket.packet_size) == false)
	{
		log_msg_.push_back("Broadcast fail ID : " + std::to_string(net_id));
		return;
	}

	// 로비 메니저에서 삭제한다.
	this->lobby_->DeletePlayer(net_id);
}

void ServerProgram::OnPacketProcessEnterChatSer(NetworkObjectID net_id, void* C2S_packet, int C2S_packet_len)
{
	MSG_C2S_ENTER_CHAT_SER* C2SRcvPacket = static_cast<MSG_C2S_ENTER_CHAT_SER*>(C2S_packet);

	MSG_S2C_ENTER_CHAT_SER_OK S2CSendPacket;

	S2CSendPacket.packet_size = sizeof(MSG_S2C_ENTER_CHAT_SER_OK);
	S2CSendPacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::S2C_ENTER_CHAT_SER_OK);
	S2CSendPacket.seqNum = 0;

	char name[MAX_NICKNAME_LEN + 1];
	memcpy(name, C2SRcvPacket->NickName, sizeof(C2SRcvPacket->NickName));//MAX_NICKNAME_LEN + 1);


	if (this->lobby_->SetPlayerNickName(net_id, name))//name))
	{
		this->lobby_->SetPlayerState(net_id, LobbyPlayerState::InChat);
		S2CSendPacket.Result = true;

		if (Send(net_id, &S2CSendPacket, S2CSendPacket.packet_size) == false)
		{
			log_msg_.push_back("Enter Chat send fail ID : " + std::to_string(net_id));
			return;
		}

		// 새로운 클라이언트가 접속했음을 전체에게 알린다.
		MSG_S2C_NEW_PLAYER S2CBroadCasePacket;

		S2CBroadCasePacket.packet_size = sizeof(MSG_S2C_NEW_PLAYER);
		S2CBroadCasePacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::S2C_NEW_PLAYER);
		S2CBroadCasePacket.seqNum = 0;
		S2CBroadCasePacket.NetID = net_id;

		std::string str = this->lobby_->GetNickName(net_id);
		const char* cstr = str.c_str();
		char* name = const_cast<char*>(cstr);

		memcpy(S2CBroadCasePacket.NickName, name, MAX_NICKNAME_LEN + 1);

		if (Broadcast(&S2CBroadCasePacket, S2CBroadCasePacket.packet_size) == false)
		{
			log_msg_.push_back("Broadcast fail ID : " + std::to_string(net_id));
			return;
		}
	}
	else
	{
		S2CSendPacket.Result = false;

		if (Send(net_id, &S2CSendPacket, S2CSendPacket.packet_size) == false)
		{
			log_msg_.push_back("send fail ID : " + std::to_string(net_id));
			return;
		}
	}
}

void ServerProgram::OnPacketProcessChangeName(NetworkObjectID net_id, void* C2S_packet, int C2S_packet_len)
{
	MSG_C2S_CHANGE_NAME* C2SRcvPacket = static_cast<MSG_C2S_CHANGE_NAME*>(C2S_packet);
	char name[MAX_NICKNAME_LEN + 1];
	memcpy(name, C2SRcvPacket->NickName, MAX_NICKNAME_LEN + 1);


	if (this->lobby_->SetPlayerNickName(net_id, name))
	{
		this->lobby_->SetPlayerState(net_id, LobbyPlayerState::InChat);

		// 이름이 변경되었음을 전체에게 알린다.
		MSG_S2C_CHANGE_NAME_OK S2CBroadCastPacket;

		S2CBroadCastPacket.packet_size = sizeof(MSG_S2C_CHANGE_NAME_OK);
		S2CBroadCastPacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::S2C_CHANGE_NAME_OK);
		S2CBroadCastPacket.seqNum = 0;
		S2CBroadCastPacket.NetID = net_id;
		memcpy(S2CBroadCastPacket.NickName, name, MAX_NICKNAME_LEN + 1);

		if (false == Broadcast(&S2CBroadCastPacket, S2CBroadCastPacket.packet_size))
		{
			log_msg_.push_back("Change Name Broadcast fail ID : " + std::to_string(net_id));
			return;
		}
	}
	else
	{
		log_msg_.push_back("Change Name fail ID : " + std::to_string(net_id));
		/*if (false == LobbyServer.Send(NetId, reinterpret_cast<char*>(&S2CSendPacket), S2CSendPacket.packet_size))
		{
			CONSOLE_PRINT("Server::%d LobbyServer.Send 실패\n", (int)NetId);
			return;
		}*/
	}
}

void ServerProgram::OnPacketProcessChat(NetworkObjectID net_id, void* C2S_packet, int C2S_packet_len)
{
	MSG_C2S_CHAT* C2SRcvPacket = static_cast<MSG_C2S_CHAT*>(C2S_packet);

	MSG_S2C_CHAT S2CBroadCasePacket;
	S2CBroadCasePacket.packet_size = sizeof(MSG_S2C_CHAT);
	S2CBroadCasePacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::S2C_CHAT);
	S2CBroadCasePacket.seqNum = 0;
	S2CBroadCasePacket.NetID = net_id;

	std::string str = this->lobby_->GetNickName(net_id);
	const char* cstr = str.c_str();
	char* name = const_cast<char*>(cstr);

	memcpy(S2CBroadCasePacket.NickName, name, MAX_NICKNAME_LEN + 1);

	memcpy(S2CBroadCasePacket.MSG, C2SRcvPacket->MSG, MAX_CHATTING_LEN + 1);

	if (false == Broadcast(&S2CBroadCasePacket, S2CBroadCasePacket.packet_size))
	{
		log_msg_.push_back("Process Chat Broadcast fail ID : " + std::to_string(net_id));
		return;
	}
}

