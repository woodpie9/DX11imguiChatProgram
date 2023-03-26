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
	// ���� ��ü�� �̸� ��������. ���� �ڵ��� ���µ� ���� �ƴ�
	for (int i = 0; i < MAXIMUM_SOCKET; i++)
	{
		m_socket_ptr_table[i] = new woodnet::TCPSocket;
	}
	m_listen_port = listemPort;

	m_listen_ptr = m_socket_ptr_table[LISTEN_SOCKET_INDEX];

	// �κ� �Ŵ��� ����
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
	m_log_msg.push_back("���� ����" + m_listen_ip + ":" + std::to_string(m_listen_port));

	// Generator�� ���ڸ� ������Ű��! ������ 0��. ���ڸ� �����ϰ� ������ ������Ų��.
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
	// �ð���Ⱑ WSA_INFINITE �� ������ Ŭ�� �޽����� �Ⱥ�������. �ð��� CommonDefines ���� ����.
	if ((socketIndex = WSAWaitForMultipleEvents(m_socket_cnt, m_event_table, FALSE, DELEY_SERVER_ms, FALSE)) == WSA_WAIT_FAILED)
	{
		m_log_msg.emplace_back("WSAWaitForMultipleEvents() failed");
		return false;
	}
	//	else
	//	CONSOLE_PRINT("WSAWaitForMultipleEvents() is pretty damn OK!\n");
	//	WSAWaitForMultipleEvents �� � �̺�Ʈ�� �������� �˷���.

	//	���� ��ü �迭�� �ε����� �̺�Ʈ �迭�� �ε����� 1-1 ����(��ġ ����)�ϵ��� �������.
	woodnet::TCPSocket* pThisSocket = m_socket_ptr_table[socketIndex];

	// �̺�Ʈ�� ���� ��
	if (pThisSocket != nullptr)
	{
		//WSAEVENT hEvent = m_EventTable[socketIndex]; �� pThisSocket->GetEventHandle(); ���� ��
		//WSAEventSelect �� ���ϰ� �̺�Ʈ ��ü�� ����. ��ü������ � ��Ʈ��ũ �̺�Ʈ���� �˻�
		m_log_msg.push_back("Current Socket is" + std::to_string(static_cast<int>(pThisSocket->GetNetID())));

		WSANETWORKEVENTS NetworkEvents;
		if (WSAEnumNetworkEvents(pThisSocket->GetHandle(), pThisSocket->GetEventHandle(), &NetworkEvents) == SOCKET_ERROR)
		{
			m_log_msg.push_back("WSAEnumNetworkEvents() failed with error" + std::to_string(WSAGetLastError()));
			return false;
		}
		//	else
		//		CONSOLE_PRINT("Server::%d WSAEnumNetworkEvents() should be fine!\n", m_listenPort);

		// ���ο� ������ �߻���
		if (NetworkEvents.lNetworkEvents & FD_ACCEPT)
		{
			if (NetworkEvents.iErrorCode[FD_ACCEPT_BIT] != 0)
			{
				m_log_msg.push_back("WSAEnumNetworkEvents() failed with error" + std::to_string(WSAGetLastError()));
				return false;
			}
			on_net_accept(pThisSocket);
		}

		// HAVE TO DO: ���� ���� ���� ó��
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


	// ��� ���ϵ��� ť�� �ִ� �޽����� ������.
	for (int i = 1; i < m_socket_cnt; i++)
	{
		if (m_socket_ptr_table[i]->SendUpdate() == false)
		{
			// Ư�� ���Ͽ��� ������
			m_log_msg.push_back("FD_READ failed with error " + std::to_string(static_cast<int>(m_socket_ptr_table[i]->GetNetID())));
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

bool ServerProgram::send(NetworkObjectID NetworkID, void* pPacket, int PacketLen)
{
	// ��ü ���� ���� �߿���
	for (int i = 1; i < m_socket_cnt; i++)
	{
		// �������� ������ ID�� ���� ���Ͽ��� ��Ŷ�� Post �Ѵ�.
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

	//HAVE TO DO: �Ʒ� NULL �� �ƴ϶� ������ Ŭ���� �ּ� �޾ƿ��� -> �׷��� ��� �����ߴ��� �ȴ�.
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
	//pNewSocket �����ϴ� ��츦 ����ϸ� Ŭ��� ���� �����ؾ� ��
	pNewSocket->Attach(newClientSocket);
	pNewSocket->SetNetID(net_id_generator());


	if (false == pNewSocket->EventSelect(FD_READ | FD_WRITE | FD_CLOSE))
	{
		m_log_msg.emplace_back("EventSelect Fail");
		closesocket(newClientSocket); // ���� ó�� �Ͽ�ȭ�ϰ� �;����� ������
		return;
	}

	// EventSelect�� �ؾ� �̺�Ʈ�� ���� ��
	m_event_table[m_socket_cnt - 1] = pNewSocket->GetEventHandle();
	m_log_msg.emplace_back("WSAEventSelect() is OK!");
	m_log_msg.emplace_back("Socket got connected..." + std::to_string((int)pNewSocket->GetNetID()));


	// �κ� �Ŵ������� ������ �Ѱܼ� �÷��̾� ��ü�� �����
	this->lobby->new_player(pNewSocket->GetNetID());

	// ��Ŷ�� ���� Ŭ�󿡰� ������ �Ϸ�Ǿ��ٰ� �˸���.
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
	//HAVE TO DO : ���� �̺�Ʈ�� Close �� ���� ��. Ŭ�� �ϳ��� �ݰ� �����Ѵ�. 

	// Ŭ���̾�Ʈ�� �������� ��ü���� �˸���.
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


	// LobbyManager������ �����.
	// Ŭ�� ���� �����̸� �̹� �������ְ����� �׷��� �ѹ� �� Ȯ���Ѵ�.
	this->lobby->delete_player(pThisSocket->GetNetID());

	// ���������� �˸���.
	MSG_S2C_LEAVE_LOBBY_OK S2CSendPacket;

	S2CSendPacket.packet_size = sizeof(MSG_S2C_LEAVE_LOBBY_OK);
	S2CSendPacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::S2C_LEAVE_LOBBY_OK);
	S2CSendPacket.seqNum = 0;

	if (send(pThisSocket->GetNetID(), &S2CSendPacket, S2CSendPacket.packet_size) == false)
	{
		m_log_msg.push_back("LobbyServer.Send fail ID : " + std::to_string(pThisSocket->GetNetID()));
	}


	// SocketCnt�ٲٱ�, SocketPtr ����. (���� �ڿ� �ִ� Ŭ�� ���۳� ������ �ű��)
	m_log_msg.push_back("Soccket FD_CLOSE! ID : " + std::to_string(pThisSocket->GetNetID()));

	int CloseSocIndex = socketIndex;
	int LastSocNetID = m_socket_cnt - 1;

	// ������ ���� �ƴϸ� ���ϰ� �̺�Ʈ�� ��ġ�� �ٲ۴�.
	// TODO :: ������ �̵��� �ƴ϶� ������ ���� ��ȣ�ڸ��� ���ο� ������ �ִ� ������ �ٲ㺼��?
	if (CloseSocIndex != LastSocNetID)
	{
		// Event Table�� SocketArray�� ���� ������ ��ȿ���� ��ġ�� �ٲ۴�.
		Swap(m_event_table[CloseSocIndex], m_event_table[LastSocNetID]);
		Swap(m_socket_ptr_table[CloseSocIndex], m_socket_ptr_table[LastSocNetID]);
	}

	/// TODO : Reset ����
	// ������ ����(������ ��)�� �ε��� ���� �����Ѵ�.
	m_socket_ptr_table[LastSocNetID]->Close();
	m_socket_ptr_table[LastSocNetID]->Reset();
	m_socket_cnt--;

	/// TODO : NetID�� �����ؾ� �Ѵ�.
	/// m_NetID--;
}

void ServerProgram::on_net_receive(woodnet::TCPSocket* pThisSocket)
{
	m_log_msg.emplace_back("FD_READ is OK!");

	// ���� ���ۿ� ���� ���ŵ� �����͸� ť�� �״´�. �׸��� ���� �ٽ� ��û
	pThisSocket->RecvUpdate();

	// ���� ť�� ��Ŷ�� �����´�.
	PACKET_HEADER msgHeader;
	while (pThisSocket->PeekPacket(reinterpret_cast<char*>(&msgHeader), sizeof(PACKET_HEADER)))
	{
		// ������ ��Ŷ�� �ִٸ� ��Ŷ�� ���� ó��.
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
	// ó�� ���� �غ� �Ǹ� �̺�Ʈ �߻�., �߻� ������ Ȯ���� ���ô�.
	// �۽�: send(buf, size... )  ȣ���ϸ� ���� ���ۿ� ���� ����Ÿ�� ����ȴ�.
	// ���� ���ۿ� ���縦 ���ϴ� ��� -> ������ ��ȿ���� ����. 
	// ���۰� ����.���۰� ������ ������ �ٽ� ��ȣ
}

void ServerProgram::get_client_IPPort(SOCKET ClientSocket, char* ipPort)
{
	/// ������ Ŭ���̾�Ʈ ������ �޾ƿͺ���.  �Լ�ȭ �ʿ�
	// ������ ���ڷ� �ְ�... ���� ����....  ��Ʈ�� �����Ͽ� �迭�� �ѹ��� ���ϱ�..?
	char addrString[IPPORT_LEN];		// ������ ũ��� IPv4 �ּҴ� 16�� �̻�. IPv6 �ּҴ� 46�� �̻�.
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

	// PortNum ��ȣ�� ������.
	char PortNum[30];
	PortNum[0] = 0;
	sprintf_s(PortNum, "%d", htons(m_ipv4Endpoint.sin_port));
	strcat_s(addrString, sizeof(addrString), PortNum);

	memcpy(ipPort, addrString, IPPORT_LEN);
}

void ServerProgram::packet_dispatcher(woodnet::TCPSocket* pRecvSocket, void* pPacket, int len)
{
	// ��Ŷ�� ���� Ŭ���̾�Ʈ�� �����Ű�

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
	// �κ��� �÷��̾� ���¸� �����մϴ�.
	this->lobby->set_player_state(NetId, LobbyPlayerState::InLobby);

	// Ŭ�󿡰� ���� ��Ŷ ����
		// Ŭ�󿡰� ���� ��Ŷ ����
	MSG_S2C_ENTER_LOBBY_OK S2CSendPacket;

	S2CSendPacket.packet_size = sizeof(MSG_S2C_ENTER_LOBBY_OK);
	S2CSendPacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::S2C_ENTER_LOBBY_OK);
	S2CSendPacket.seqNum = 0;

	// ������. SendFlush�� Update ���� �ϴܿ� ����.
	if (send(NetId, &S2CSendPacket, S2CSendPacket.packet_size) == false)
	{
		m_log_msg.push_back("LobbyServer.Send fail ID : " + std::to_string(NetId));
		return;
	}
}

void ServerProgram::on_packet_proc_leave_lobby(NetworkObjectID NetId)
{
	// Ŭ�󿡰� ���� ��Ŷ ����
	MSG_S2C_LEAVE_LOBBY_OK S2CSendPacket;

	S2CSendPacket.packet_size = sizeof(MSG_S2C_LEAVE_LOBBY_OK);
	S2CSendPacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::S2C_LEAVE_LOBBY_OK);
	S2CSendPacket.seqNum = 0;

	// ������. SendFlush�� Update ���� �ϴܿ� ����.
	if (send(NetId, reinterpret_cast<char*>(&S2CSendPacket), S2CSendPacket.packet_size) == false)
	{
		m_log_msg.push_back("LobbyServer.Send fail ID : " + std::to_string(NetId));
		return;
	}


	// Ŭ���̾�Ʈ�� �������� ��ü���� �˸���.
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

	// �κ� �޴������� �����Ѵ�.
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

		// ���ο� Ŭ���̾�Ʈ�� ���������� ��ü���� �˸���.
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

		// �̸��� ����Ǿ����� ��ü���� �˸���.
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
			CONSOLE_PRINT("Server::%d LobbyServer.Send ����\n", (int)NetId);
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

