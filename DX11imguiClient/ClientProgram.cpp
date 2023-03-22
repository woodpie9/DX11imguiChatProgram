#include <WS2tcpip.h>
#include "ClientProgram.h"
#include "../WoodnetBase/WoodnetProtocol.h"

static constexpr int PACKET_HEADER_SIZE = sizeof(PACKET_HEADER);

ClientProgram::ClientProgram() : OnClient(false), OnConnect(false)
{
	set_last_error(_Error::success);
}

bool ClientProgram::init()
{
	m_vector_msg.emplace_back("init client...");

	// �̹� init �Ϸ���
	if (OnClient)
	{
		m_vector_msg.emplace_back("already_init");
		set_last_error(_Error::already_init);
		return false;
	}

	// �̹� ���� �Ǿ�����
	if (OnConnect)
	{
		m_vector_msg.emplace_back("already_connect");
		set_last_error(_Error::already_connect);
		return false;
	}

	// ������ ����.
	if (m_Connector.Open(IPPROTO_TCP) == false)
	{
		m_vector_msg.emplace_back("socket open fail");
		set_last_error(_Error::fatal);
		OnConnect = false;
		return false;
	}
	set_connection_status(ConnectionStatus::Oppend);

	// ���Ͽ� ���� �����ִ� �̺�Ʈ�� �����Ѵ�.
	if (m_Connector.EventSelect(FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE) == false)
	{
		m_vector_msg.emplace_back("eventselect fail");
		set_last_error(_Error::fatal);
		OnConnect = false;
		return false;
	}
	set_connection_status(ConnectionStatus::SetEvent);


	OnClient = true;

	set_last_error(_Error::success);
	return  true;
}

bool ClientProgram::connect_server(std::string ConnectIP)
{
	bool result = false;
	m_vector_msg.emplace_back("connecting server...");

	// Init �ʿ�
	if (!OnClient)
	{
		m_vector_msg.emplace_back("not_init");
		set_last_error(_Error::not_init);
		return false;
	}

	// �̹� ���� �Ǿ�����
	if (OnConnect)
	{
		m_vector_msg.emplace_back("already_connect");
		set_last_error(_Error::already_connect);
		return false;
	}

	// ���Ͽ� ������ ���ε� �Ѵ�.
	SOCKADDR_IN ConnectAddr;
	ConnectAddr.sin_family = AF_INET;
	InetPtonA(AF_INET, ConnectIP.c_str(), &ConnectAddr.sin_addr);
	ConnectAddr.sin_port = htons(m_port);

	// ������ ������ ������ �����Ѵ�.
	if (m_Connector.Connect(ConnectAddr) == false)
	{
		m_vector_msg.emplace_back("connect fail");
		set_last_error(_Error::fatal);
		OnConnect = false;
		return false;
	}

	// �̺�Ʈ�� ����Ѵ�.
	m_Event = m_Connector.GetEventHandle();

	m_vector_msg.push_back("Connect to " + ConnectIP + ":" + std::to_string(m_port));
	set_connection_status(ConnectionStatus::Connecting);

	OnConnect = true;
	return true;
}

bool ClientProgram::login_server()
{
	if (check_connect() == false) return false;

	m_vector_msg.emplace_back("Login server...");

	static MSG_C2S_ENTER_LOBBY C2SSendPacket;

	C2SSendPacket.packet_size = sizeof(MSG_C2S_ENTER_LOBBY);
	C2SSendPacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::C2S_ENTER_LOBBY);
	C2SSendPacket.seqNum = 1;

	m_Connector.PostPacket(reinterpret_cast<char*>(&C2SSendPacket), C2SSendPacket.packet_size);

	return true;
}

bool ClientProgram::set_nickname(std::string nickname)
{
	if (check_connect() == false) return false;

	m_vector_msg.emplace_back("Set Nickname...");

	static MSG_C2S_ENTER_CHAT_SER C2SSendPacket;

	C2SSendPacket.packet_size = sizeof(MSG_C2S_ENTER_CHAT_SER);
	C2SSendPacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::C2S_ENTER_CHAT_SER);
	C2SSendPacket.seqNum = 1;

	const char* cstr = nickname.c_str();
	memcpy(C2SSendPacket.NickName, cstr, MAX_NICKNAME_LEN + 1);

	m_Connector.PostPacket((char*)&C2SSendPacket, C2SSendPacket.packet_size);

	return true;
}

bool ClientProgram::change_name(std::string nickname)
{
	if (check_connect() == false) return false;

	m_vector_msg.emplace_back("Change Nickname...");

	static MSG_C2S_CHANGE_NAME C2SSendPacket;

	C2SSendPacket.packet_size = sizeof(MSG_C2S_CHANGE_NAME);
	C2SSendPacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::C2S_CHANGE_NAME);
	C2SSendPacket.seqNum = 1;

	const char* cstr = nickname.c_str();
	memcpy(C2SSendPacket.NickName, cstr, MAX_NICKNAME_LEN + 1);

	m_Connector.PostPacket(reinterpret_cast<char*>(&C2SSendPacket), C2SSendPacket.packet_size);

	return true;
}

bool ClientProgram::network_update()
{
	if (check_connect() == false) return false;

	// ���� ���� �̺�Ʈ�� �ִ��� Ȯ���Ѵ�.
	static int EventReturn = 0;
	if ((EventReturn = WSAWaitForMultipleEvents(1, &m_Event, FALSE, 1, FALSE)) == WSA_WAIT_FAILED)
	{
		m_vector_msg.push_back("Client::WSAWaitForMultipleEvents() failed with error " + std::to_string(WSAGetLastError()));
		set_last_error(_Error::fatal);
		return false;
	}

	if (EventReturn == WSA_WAIT_TIMEOUT)
	{
		// Ÿ�Ӿƿ��̸� �������� �Ѱ��ش�.
		// SendFlush�� TimeOut�� �ұ�?
		send_flush();
		set_last_error(_Error::success);
		return true;
	}

	// �̹��� ���� �̺�Ʈ�� ����� Ȯ���Ѵ�.
	WSANETWORKEVENTS NetworkEvents;
	if (WSAEnumNetworkEvents(m_Connector.GetHandle(), m_Connector.GetEventHandle(), &NetworkEvents) == SOCKET_ERROR)
	{
		m_vector_msg.push_back("Client::WSAEnumNetworkEvents() failed with error " + std::to_string(WSAGetLastError()));
		set_last_error(_Error::error);
		return false;
	}


	if (NetworkEvents.lNetworkEvents & FD_CONNECT)
	{
		if (NetworkEvents.iErrorCode[FD_CONNECT_BIT] != 0)
		{
			m_vector_msg.push_back("FD_CONNECT failed with error " + std::to_string(NetworkEvents.iErrorCode[FD_CONNECT_BIT]));
			set_last_error(_Error::error);
			return false;
		}

		on_net_connect(&m_Connector);
	}


	if (NetworkEvents.lNetworkEvents & FD_CLOSE)
	{
		if (NetworkEvents.iErrorCode[FD_CLOSE_BIT] != 0)
		{
			m_vector_msg.push_back("FD_CONNECT failed with error " + std::to_string(NetworkEvents.iErrorCode[FD_CONNECT_BIT]));
			set_last_error(_Error::error);
			return false;
		}

		on_net_connect(&m_Connector);
	}


	if (NetworkEvents.lNetworkEvents & FD_READ)
	{
		if (NetworkEvents.iErrorCode[FD_READ_BIT] != 0)
		{
			m_vector_msg.push_back("FD_CONNECT failed with error " + std::to_string(NetworkEvents.iErrorCode[FD_CONNECT_BIT]));
			set_last_error(_Error::error);
			return false;
		}

		on_net_recv(&m_Connector);
	}


	if (NetworkEvents.lNetworkEvents & FD_WRITE)
	{
		if (NetworkEvents.iErrorCode[FD_WRITE_BIT] != 0)
		{
			m_vector_msg.push_back("FD_CONNECT failed with error " + std::to_string(NetworkEvents.iErrorCode[FD_CONNECT_BIT]));
			set_last_error(_Error::error);
			return false;
		}

		on_net_connect(&m_Connector);
	}

	return true;
}

//bool ClientProgram::clean_up()
//{
//	if (check_connect() == false) return false;
//
//	m_Connector.Close();
//
//	return false;
//}

bool ClientProgram::close()
{
	if (check_connect() == false) return false;

	m_Connector.Close();

	return true;
}

bool ClientProgram::send_chat_msg(std::string msg)
{
	if (check_connect() == false) return false;

	static MSG_C2S_CHAT C2SSendPacket;

	C2SSendPacket.packet_size = sizeof(MSG_C2S_CHAT);
	C2SSendPacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::C2S_CHAT);
	C2SSendPacket.seqNum = 1;

	const char* cMSG = msg.c_str();
	memcpy(C2SSendPacket.MSG, cMSG, MAX_CHATTING_LEN + 1);

	m_Connector.PostPacket(reinterpret_cast<char*>(&C2SSendPacket), C2SSendPacket.packet_size);


	return true;
}

bool ClientProgram::check_connect()
{
	// Init �ʿ�
	if (!OnClient)
	{
		m_vector_msg.emplace_back("not_init");
		set_last_error(_Error::not_init);
		return false;
	}

	// �̹� ���� �Ǿ�����
	if (!OnConnect)
	{
		m_vector_msg.emplace_back("not_connect");
		set_last_error(_Error::not_connect);
		return false;
	}

	return true;
}

bool ClientProgram::send(int PacketID, void* pPacket, int PacketLen)
{
	if (check_connect() == false) return false;

	if (m_Connector.PostPacket(static_cast<char*>(pPacket), PacketLen) == false)
	{
		m_vector_msg.emplace_back("PostPacket Fail");
		set_last_error(_Error::error);
		return false;
	}

	return true;
}

void ClientProgram::send_flush()
{
	m_Connector.SendUpdate();
}



// ��Ʈ��ũ �̺�Ʈ ����
bool ClientProgram::on_net_connect(woodnet::TCPSocket* pThisSocket)
{
	m_vector_msg.emplace_back("Connect is OK!");
	set_connection_status(ConnectionStatus::Connected);
	set_last_error(_Error::success);
	return true;
}

bool ClientProgram::on_net_close(woodnet::TCPSocket* pThisSocket)
{
	m_vector_msg.emplace_back("close packet");
	set_connection_status(ConnectionStatus::Closed);
	set_last_error(_Error::success);
	return true;
}

bool ClientProgram::on_net_recv(woodnet::TCPSocket* pThisSocket)
{
	static PACKET_HEADER msgHeader;
	m_Connector.RecvUpdate();

	while (m_Connector.PeekPacket(reinterpret_cast<char*>(&msgHeader), PACKET_HEADER_SIZE))
	{
		// ������ ��Ŷ�� �ִٸ� ��Ŷ�� ���� ó��.
		memset(&m_PacketRecvBuf, 0, PACKET_SIZE_MAX);

		if (msgHeader.packet_size > PACKET_SIZE_MAX || msgHeader.packet_size < 0)
		{
			m_vector_msg.push_back("WinClient::OnNetRecv packet size error " + std::to_string(msgHeader.packet_size));
			set_last_error(_Error::error);
			continue;
		}

		// ũ�Ⱑ �˸´� �ϳ��� ��Ŷ�� �о�´�.
		if (m_Connector.ReadPacket(m_PacketRecvBuf, msgHeader.packet_size))
		{
			// ��Ŷ�� �ؼ��ϰ� �˸´� ������ �Ѵ�.
			PacketDispatcher(pThisSocket, m_PacketRecvBuf, msgHeader.packet_size);
		}
		else
		{
			m_vector_msg.emplace_back("Connector.ReadPacket return false!");
			break;
		}
	}

	return true;
}

bool ClientProgram::on_net_send(woodnet::TCPSocket* pThisSocket)
{
	//TODO : �۽� ������ ���� ������ ���. ���⼱ �̻��
	return true;
}




// ��Ŷ �̺�Ʈ ����
bool ClientProgram::PacketDispatcher(woodnet::TCPSocket* RecvSock, void* pPacket, int len)
{
	PACKET_HEADER* pHeader = (PACKET_HEADER*)pPacket;

	switch (pHeader->packet_id)
	{
		case static_cast<INT16>(COMMON_PAKET_ID::S2C_ACCEPT):
		{
			return on_packet_proc_accept(RecvSock, pPacket, len);
		}
		break;

		case static_cast<INT16>(LOBBY_PACKET_ID::S2C_ENTER_LOBBY_OK):
		{
			return on_packet_proc_enter_lobby(RecvSock, pPacket, len);
		}
		break;

		case static_cast<INT16>(LOBBY_PACKET_ID::S2C_ENTER_CHAT_SER_OK):
		{
			return on_packet_proc_enter_chat(RecvSock, pPacket, len);
		}
		break;

		case static_cast<INT16>(LOBBY_PACKET_ID::S2C_NEW_PLAYER):
		{
			return on_packet_proc_new_player(RecvSock, pPacket, len);
		}
		break;

		case static_cast<INT16>(LOBBY_PACKET_ID::S2C_LEAVE_PLAYER):
		{
			return on_packet_proc_leave_player(RecvSock, pPacket, len);
		}
		break;

		case static_cast<INT16>(LOBBY_PACKET_ID::S2C_CHANGE_NAME_OK):
		{
			return on_packet_proc_change_name(RecvSock, pPacket, len);
		}
		break;

		case static_cast<INT16>(LOBBY_PACKET_ID::S2C_CHAT):
		{
			return on_packet_proc_enter_chat(RecvSock, pPacket, len);
		}
		break;

		case static_cast<INT16>(LOBBY_PACKET_ID::S2C_NOTICE):
		{
			return on_packet_proc_notice(RecvSock, pPacket, len);
		}
		break;

		default:
		{
			m_vector_msg.push_back("Unknown Packet! id : " + std::to_string(pHeader->packet_id) + "size : " + std::to_string(pHeader->packet_size));
			set_last_error(_Error::error);
			return false;
		}
		break;
	}
}

bool ClientProgram::on_packet_proc_accept(woodnet::TCPSocket* RecvSock, void* pS2CPacket, int C2SPacketLen)
{
	return false;
}

bool ClientProgram::on_packet_proc_enter_lobby(woodnet::TCPSocket* RecvSock, void* pS2CPacket, int C2SPacketLen)
{
	return false;
}

bool ClientProgram::on_packet_proc_enter_chat(woodnet::TCPSocket* RecvSock, void* pS2CPacket, int C2SPacketLen)
{
}

bool ClientProgram::on_packet_proc_new_player(woodnet::TCPSocket* RecvSock, void* pS2CPacket, int C2SPacketLen)
{
	return false;
}

bool ClientProgram::on_packet_proc_leave_player(woodnet::TCPSocket* RecvSock, void* pS2CPacket, int C2SPacketLen)
{
	return false;
}

bool ClientProgram::on_packet_proc_change_name(woodnet::TCPSocket* RecvSock, void* pS2CPacket, int C2SPacketLen)
{
	return false;
}

bool ClientProgram::on_packet_proc_chat(woodnet::TCPSocket* RecvSock, void* pS2CPacket, int C2SPacketLen)
{
	return false;
}

bool ClientProgram::on_packet_proc_notice(woodnet::TCPSocket* RecvSock, void* pS2CPacket, int C2SPacketLen)
{
	return false;
}
