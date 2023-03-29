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
	// ���� ��ü�� �̸� ��������. ���� �ڵ��� ���µ� ���� �ƴ�
	for (int i = 0; i < MAXIMUM_SOCKET; i++)
	{
		socket_ptr_table_[i] = new woodnet::TCPSocket;
	}
	listen_port_ = listem_port;

	listen_ptr_ = socket_ptr_table_[LISTEN_SOCKET_INDEX];

	// �κ� �Ŵ��� ����
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

	// Generator�� ���ڸ� ������Ű��! ������ 0��. ���ڸ� �����ϰ� ������ ������Ų��.
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
	// �ð���Ⱑ WSA_INFINITE �� ������ Ŭ�� �޽����� �Ⱥ�������. �ð��� CommonDefines ���� ����.
	if ((socket_index = WSAWaitForMultipleEvents(socket_cnt_, event_table_, FALSE, DELEY_SERVER_MS, FALSE)) == WSA_WAIT_FAILED)
	{
		log_msg_.emplace_back("WSAWaitForMultipleEvents() failed");
		return false;
	}

	// TODO : Event�� ������ int �� �޴°��� �ƴ� �������� ���� �޾Ƽ� ����ó���� �ؾ� �Ѵ�
	if (socket_index == WSA_WAIT_TIMEOUT) // 258 �̸�...
	{
		// index�� 258�϶� pthisSocket�� null�� �ȶߴ� ��찡 �߻���... ����?
		// �ӽ� ó��
		return true;
	}

	//	else
	//	CONSOLE_PRINT("WSAWaitForMultipleEvents() is pretty damn OK!\n");
	//	WSAWaitForMultipleEvents �� ���(��� ���Ͽ���) � �̺�Ʈ�� �������� �˷���.

	//	���� ��ü �迭�� �ε����� �̺�Ʈ �迭�� �ε����� 1-1 ����(��ġ ����)�ϵ��� �������.
	woodnet::TCPSocket* this_socket = socket_ptr_table_[socket_index];

	// �̺�Ʈ�� ���� ��
	if (this_socket != nullptr)
	{
		//WSAEVENT hEvent = m_EventTable[socketIndex]; �� pThisSocket->GetEventHandle(); ���� ��
		//WSAEventSelect �� ���ϰ� �̺�Ʈ ��ü�� ����. ��ü������ � ��Ʈ��ũ �̺�Ʈ���� �˻�
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

		// ���ο� ������ �߻���
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

		// HAVE TO DO: ���� ���� ���� ó��
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


	// ��� ���ϵ��� ť�� �ִ� �޽����� ������.
	for (int i = 1; i < socket_cnt_; i++)
	{
		if (socket_ptr_table_[i]->SendUpdate() == false)
		{
			// Ư�� ���Ͽ��� ������
			log_msg_.push_back("FD_READ failed with error " 
				+ std::to_string(static_cast<int>(socket_ptr_table_[i]->GetNetId())));
		}
	}

	return true;
}

//bool ServerProgram::close()
//{
//	// close�� clean_up �� ������ �ؾ���..��?
//
//	return false;
//}

bool ServerProgram::Send(NetworkObjectID NetworkID, void* pPacket, int PacketLen)
{
	// ��ü ���� ���� �߿���
	for (int i = 1; i < socket_cnt_; i++)
	{
		// �������� ������ ID�� ���� ���Ͽ��� ��Ŷ�� Post �Ѵ�.
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

	//HAVE TO DO: �Ʒ� NULL �� �ƴ϶� ������ Ŭ���� �ּ� �޾ƿ��� -> �׷��� ��� �����ߴ��� �ȴ�.
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
	//pNewSocket �����ϴ� ��츦 ����ϸ� Ŭ��� ���� �����ؾ� ��
	new_socket->Attach(new_client_socket);
	new_socket->SetNetId(NetIdGenerator());


	if (false == new_socket->EventSelect(FD_READ | FD_WRITE | FD_CLOSE))
	{
		log_msg_.emplace_back("EventSelect Fail");
		closesocket(new_client_socket); // ���� ó�� �Ͽ�ȭ�ϰ� �;����� ������
		return;
	}

	// EventSelect�� �ؾ� �̺�Ʈ�� ���� ��
	event_table_[socket_cnt_ - 1] = new_socket->GetEventHandle();
	log_msg_.emplace_back("WSAEventSelect() is OK!");
	log_msg_.emplace_back("Socket got connected..." 
		+ std::to_string((int)new_socket->GetNetId()));


	// �κ� �Ŵ������� ������ �Ѱܼ� �÷��̾� ��ü�� �����
	this->lobby_->NewPlayer(new_socket->GetNetId());

	// ��Ŷ�� ���� Ŭ�󿡰� ������ �Ϸ�Ǿ��ٰ� �˸���.
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
	//HAVE TO DO : ���� �̺�Ʈ�� Close �� ���� ��. Ŭ�� �ϳ��� �ݰ� �����Ѵ�. 

	// Ŭ���̾�Ʈ�� �������� ��ü���� �˸���.
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


	// LobbyManager������ �����.
	// Ŭ�� ���� �����̸� �̹� �������ְ����� �׷��� �ѹ� �� Ȯ���Ѵ�.
	this->lobby_->DeletePlayer(this_socket->GetNetId());

	// ���������� �˸���.
	MSG_S2C_LEAVE_LOBBY_OK S2CSendPacket;

	S2CSendPacket.packet_size = sizeof(MSG_S2C_LEAVE_LOBBY_OK);
	S2CSendPacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::S2C_LEAVE_LOBBY_OK);
	S2CSendPacket.seqNum = 0;

	if (Send(this_socket->GetNetId(), &S2CSendPacket, S2CSendPacket.packet_size) == false)
	{
		log_msg_.push_back("LobbyServer.Send fail ID : " 
			+ std::to_string(this_socket->GetNetId()));
	}


	// SocketCnt�ٲٱ�, SocketPtr ����. (���� �ڿ� �ִ� Ŭ�� ���۳� ������ �ű��)
	log_msg_.push_back("Soccket FD_CLOSE! ID : " 
		+ std::to_string(this_socket->GetNetId()));

	int CloseSocIndex = socket_index;
	int LastSocNetID = socket_cnt_ - 1;

	// ������ ���� �ƴϸ� ���ϰ� �̺�Ʈ�� ��ġ�� �ٲ۴�.
	// TODO :: ������ �̵��� �ƴ϶� ������ ���� ��ȣ�ڸ��� ���ο� ������ �ִ� ������ �ٲ㺼��?
	if (CloseSocIndex != LastSocNetID)
	{
		// Event Table�� SocketArray�� ���� ������ ��ȿ���� ��ġ�� �ٲ۴�.
		Swap(event_table_[CloseSocIndex], event_table_[LastSocNetID]);
		Swap(socket_ptr_table_[CloseSocIndex], socket_ptr_table_[LastSocNetID]);
	}

	/// TODO : Reset ����
	// ������ ����(������ ��)�� �ε��� ���� �����Ѵ�.
	socket_ptr_table_[LastSocNetID]->Close();
	socket_ptr_table_[LastSocNetID]->Reset();
	socket_cnt_--;

	/// TODO : NetID�� �����ؾ� �Ѵ�.
	/// m_NetID--;
}

void ServerProgram::OnNetReceive(woodnet::TCPSocket* this_socket)
{
	log_msg_.emplace_back("FD_READ is OK!");

	// ���� ���ۿ� ���� ���ŵ� �����͸� ť�� �״´�. �׸��� ���� �ٽ� ��û
	this_socket->RecvUpdate();

	// ���� ť�� ��Ŷ�� �����´�.
	PACKET_HEADER msgHeader;
	while (this_socket->PeekPacket(reinterpret_cast<char*>(&msgHeader), sizeof(PACKET_HEADER)))
	{
		// ������ ��Ŷ�� �ִٸ� ��Ŷ�� ���� ó��.
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
	// ó�� ���� �غ� �Ǹ� �̺�Ʈ �߻�., �߻� ������ Ȯ���� ���ô�.
	// �۽�: send(buf, size... )  ȣ���ϸ� ���� ���ۿ� ���� ����Ÿ�� ����ȴ�.
	// ���� ���ۿ� ���縦 ���ϴ� ��� -> ������ ��ȿ���� ����. 
	// ���۰� ����.���۰� ������ ������ �ٽ� ��ȣ
}

void ServerProgram::GetClientIpPort(SOCKET client_socket, char* ip_port)
{
	/// ������ Ŭ���̾�Ʈ ������ �޾ƿͺ���.  �Լ�ȭ �ʿ�
	// ������ ���ڷ� �ְ�... ���� ����....  ��Ʈ�� �����Ͽ� �迭�� �ѹ��� ���ϱ�..?
	char addrString[IPPORT_LEN];		// ������ ũ��� IPv4 �ּҴ� 16�� �̻�. IPv6 �ּҴ� 46�� �̻�.
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

	// PortNum ��ȣ�� ������.
	char PortNum[30];
	PortNum[0] = 0;
	sprintf_s(PortNum, "%d", htons(m_ipv4Endpoint.sin_port));
	strcat_s(addrString, sizeof(addrString), PortNum);

	memcpy(ip_port, addrString, IPPORT_LEN);
}

void ServerProgram::PacketDispatcher(woodnet::TCPSocket* recv_socket, void* packet, int len)
{
	// ��Ŷ�� ���� Ŭ���̾�Ʈ�� �����Ű�

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
	// �κ��� �÷��̾� ���¸� �����մϴ�.
	this->lobby_->SetPlayerState(net_id, LobbyPlayerState::InLobby);

	// Ŭ�󿡰� ���� ��Ŷ ����
	MSG_S2C_ENTER_LOBBY_OK S2CSendPacket;

	S2CSendPacket.packet_size = sizeof(MSG_S2C_ENTER_LOBBY_OK);
	S2CSendPacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::S2C_ENTER_LOBBY_OK);
	S2CSendPacket.seqNum = 0;

	// ������. SendFlush�� Update ���� �ϴܿ� ����.
	if (Send(net_id, &S2CSendPacket, S2CSendPacket.packet_size) == false)
	{
		log_msg_.push_back("Enter Lobby Send fail ID : " + std::to_string(net_id));
		return;
	}
}

void ServerProgram::OnPacketProcessLeaveLobby(NetworkObjectID net_id)
{
	// Ŭ�󿡰� ���� ��Ŷ ����
	MSG_S2C_LEAVE_LOBBY_OK S2CSendPacket;

	S2CSendPacket.packet_size = sizeof(MSG_S2C_LEAVE_LOBBY_OK);
	S2CSendPacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::S2C_LEAVE_LOBBY_OK);
	S2CSendPacket.seqNum = 0;

	// ������. SendFlush�� Update ���� �ϴܿ� ����.
	if (Send(net_id, reinterpret_cast<char*>(&S2CSendPacket), S2CSendPacket.packet_size) == false)
	{
		log_msg_.push_back("Leave Lobby Send fail ID : " + std::to_string(net_id));
		return;
	}


	// Ŭ���̾�Ʈ�� �������� ��ü���� �˸���.
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

	// �κ� �޴������� �����Ѵ�.
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

		// ���ο� Ŭ���̾�Ʈ�� ���������� ��ü���� �˸���.
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

		// �̸��� ����Ǿ����� ��ü���� �˸���.
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
			CONSOLE_PRINT("Server::%d LobbyServer.Send ����\n", (int)NetId);
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

