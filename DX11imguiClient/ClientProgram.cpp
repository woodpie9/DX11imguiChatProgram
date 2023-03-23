
#include "ClientProgram.h"
#include "../WoodnetBase/WoodnetProtocol.h"



static constexpr int PACKET_HEADER_SIZE = sizeof(PACKET_HEADER);

ClientProgram::ClientProgram() : OnClient(false), OnConnect(false)
{
	set_last_error(_Error::success);
}

bool ClientProgram::init()
{
	m_log_msg.emplace_back("init client...");

	// 이미 init 완료함
	if (OnClient)
	{
		m_log_msg.emplace_back("already_init");
		set_last_error(_Error::already_init);
		return false;
	}

	// 이미 연결 되어있음
	if (OnConnect)
	{
		m_log_msg.emplace_back("already_connect");
		set_last_error(_Error::already_connect);
		return false;
	}

	// 소켓을 연다.
	if (m_Connector.Open(IPPROTO_TCP) == false)
	{
		m_log_msg.emplace_back("socket open fail");
		set_last_error(_Error::fatal);
		OnConnect = false;
		return false;
	}
	set_connection_status(ConnectionStatus::Oppend);

	// 소켓에 내가 관심있는 이벤트를 지정한다.
	if (m_Connector.EventSelect(FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE) == false)
	{
		m_log_msg.emplace_back("eventselect fail");
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
	m_log_msg.emplace_back("connecting server...");

	// Init 필요
	if (!OnClient)
	{
		m_log_msg.emplace_back("not_init");
		set_last_error(_Error::not_init);
		return false;
	}

	// 이미 연결 되어있음
	if (OnConnect)
	{
		m_log_msg.emplace_back("already_connect");
		set_last_error(_Error::already_connect);
		return false;
	}

	// 소켓에 정보를 바인딩 한다.
	SOCKADDR_IN ConnectAddr;
	ConnectAddr.sin_family = AF_INET;
	InetPtonA(AF_INET, ConnectIP.c_str(), &ConnectAddr.sin_addr);
	ConnectAddr.sin_port = htons(m_port);

	// 소켓의 정보로 서버에 연결한다.
	if (m_Connector.Connect(ConnectAddr) == false)
	{
		m_log_msg.emplace_back("connect fail");
		set_last_error(_Error::fatal);
		OnConnect = false;
		return false;
	}

	// 이벤트를 등록한다.
	m_Event = m_Connector.GetEventHandle();

	m_log_msg.push_back("Connect to " + ConnectIP + ":" + std::to_string(m_port));
	set_connection_status(ConnectionStatus::Connecting);

	OnConnect = true;
	return true;
}

bool ClientProgram::login_server()
{
	if (check_connect() == false) return false;

	m_log_msg.emplace_back("Login server...");

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

	m_log_msg.emplace_back("Set Nickname...");

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

	m_log_msg.emplace_back("Change Nickname...");

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

	// 새로 들어온 이벤트가 있는지 확인한다.
	static int EventReturn = 0;
	if ((EventReturn = WSAWaitForMultipleEvents(1, &m_Event, FALSE, 1, FALSE)) == WSA_WAIT_FAILED)
	{
		m_log_msg.push_back("WSAWaitForMultipleEvents() failed with error " + std::to_string(WSAGetLastError()));
		set_last_error(_Error::fatal);
		return false;
	}

	if (EventReturn == WSA_WAIT_TIMEOUT)
	{
		// 타임아웃이면 통제권을 넘겨준다.
		// SendFlush를 TimeOut때 할까?
		send_flush();
		set_last_error(_Error::success);
		return true;
	}

	// 이번에 받은 이벤트가 어떤건지 확인한다.
	WSANETWORKEVENTS NetworkEvents;
	if (WSAEnumNetworkEvents(m_Connector.GetHandle(), m_Connector.GetEventHandle(), &NetworkEvents) == SOCKET_ERROR)
	{
		m_log_msg.push_back("WSAEnumNetworkEvents() failed with error " + std::to_string(WSAGetLastError()));
		set_last_error(_Error::error);
		return false;
	}


	if (NetworkEvents.lNetworkEvents & FD_CONNECT)
	{
		if (NetworkEvents.iErrorCode[FD_CONNECT_BIT] != 0)
		{
			m_log_msg.push_back("FD_CONNECT failed with error " + std::to_string(NetworkEvents.iErrorCode[FD_CONNECT_BIT]));
			set_last_error(_Error::error);
			return false;
		}

		on_net_connect(&m_Connector);
	}


	if (NetworkEvents.lNetworkEvents & FD_CLOSE)
	{
		if (NetworkEvents.iErrorCode[FD_CLOSE_BIT] != 0)
		{
			m_log_msg.push_back("FD_CONNECT failed with error " + std::to_string(NetworkEvents.iErrorCode[FD_CONNECT_BIT]));
			set_last_error(_Error::error);
			return false;
		}

		on_net_connect(&m_Connector);
	}


	if (NetworkEvents.lNetworkEvents & FD_READ)
	{
		if (NetworkEvents.iErrorCode[FD_READ_BIT] != 0)
		{
			m_log_msg.push_back("FD_CONNECT failed with error " + std::to_string(NetworkEvents.iErrorCode[FD_CONNECT_BIT]));
			set_last_error(_Error::error);
			return false;
		}

		on_net_recv(&m_Connector);
	}


	if (NetworkEvents.lNetworkEvents & FD_WRITE)
	{
		if (NetworkEvents.iErrorCode[FD_WRITE_BIT] != 0)
		{
			m_log_msg.push_back("FD_CONNECT failed with error " + std::to_string(NetworkEvents.iErrorCode[FD_CONNECT_BIT]));
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
	// Init 필요
	if (!OnClient)
	{
		m_log_msg.emplace_back("not_init");
		set_last_error(_Error::not_init);
		return false;
	}

	// 이미 연결 되어있음
	if (!OnConnect)
	{
		m_log_msg.emplace_back("not_connect");
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
		m_log_msg.emplace_back("PostPacket Fail");
		set_last_error(_Error::error);
		return false;
	}

	return true;
}

void ClientProgram::send_flush()
{
	m_Connector.SendUpdate();
}



// 네트워크 이벤트 관리
bool ClientProgram::on_net_connect(woodnet::TCPSocket* pThisSocket)
{
	m_log_msg.emplace_back("Connect is OK!");
	set_connection_status(ConnectionStatus::Connected);
	set_last_error(_Error::success);
	return true;
}

bool ClientProgram::on_net_close(woodnet::TCPSocket* pThisSocket)
{
	m_log_msg.emplace_back("close packet");
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
		// 온전한 패킷이 있다면 패킷을 만들어서 처리.
		memset(&m_PacketRecvBuf, 0, PACKET_SIZE_MAX);

		if (msgHeader.packet_size > PACKET_SIZE_MAX || msgHeader.packet_size < 0)
		{
			m_log_msg.push_back("OnNetRecv packet size error " + std::to_string(msgHeader.packet_size));
			set_last_error(_Error::error);
			continue;
		}

		// 크기가 알맞는 하나의 패킷을 읽어온다.
		if (m_Connector.ReadPacket(m_PacketRecvBuf, msgHeader.packet_size))
		{
			// 패킷을 해석하고 알맞는 동작을 한다.
			PacketDispatcher(pThisSocket, m_PacketRecvBuf, msgHeader.packet_size);
		}
		else
		{
			m_log_msg.emplace_back("Connector.ReadPacket return false!");
			break;
		}
	}

	return true;
}

bool ClientProgram::on_net_send(woodnet::TCPSocket* pThisSocket)
{
	//TODO : 송신 가능한 상태 관리에 사용. 여기선 미사용
	return true;
}




// 패킷 이벤트 관리
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
			return on_packet_proc_chat(RecvSock, pPacket, len);
		}
		break;

		case static_cast<INT16>(LOBBY_PACKET_ID::S2C_NOTICE):
		{
			return on_packet_proc_notice(RecvSock, pPacket, len);
		}
		break;

		default:
		{
			m_log_msg.push_back("Unknown Packet! id : " + std::to_string(pHeader->packet_id) + "size : " + std::to_string(pHeader->packet_size));
			set_last_error(_Error::error);
			return false;
		}
		break;
	}
}

bool ClientProgram::on_packet_proc_accept(woodnet::TCPSocket* RecvSock, void* pS2CPacket, int C2SPacketLen)
{
	MSG_S2C_ACCEPT* S2CRcvPacket = static_cast<MSG_S2C_ACCEPT*>(pS2CPacket);

	my_net_id_ = S2CRcvPacket->NetID;
	m_log_msg.push_back("Enter Server. NetID : " + std::to_string(my_net_id_));

	return true;
}

bool ClientProgram::on_packet_proc_enter_lobby(woodnet::TCPSocket* RecvSock, void* pS2CPacket, int C2SPacketLen)
{
	m_log_msg.emplace_back("pls set nickname");

	return true;
}

bool ClientProgram::on_packet_proc_enter_chat(woodnet::TCPSocket* RecvSock, void* pS2CPacket, int C2SPacketLen)
{
	MSG_S2C_ENTER_CHAT_SER_OK* S2CRcvPacket = static_cast<MSG_S2C_ENTER_CHAT_SER_OK*>(pS2CPacket);

	bool result = S2CRcvPacket->Result;

	if (result == true)
	{
		m_log_msg.emplace_back("success Into Chat room!!!");

		return false;
	}
	else
	{
		m_log_msg.emplace_back("fail into chat room...");

		return true;
	}
}

bool ClientProgram::on_packet_proc_new_player(woodnet::TCPSocket* RecvSock, void* pS2CPacket, int C2SPacketLen)
{
	MSG_S2C_NEW_PLAYER* S2CRcvPacket = static_cast<MSG_S2C_NEW_PLAYER*>(pS2CPacket);

	std::string str = u8"새로운 클라이언트 접속. NetID : ";
	INT32 id = S2CRcvPacket->NetID;
	str += std::to_string(id);

	str += " Name : ";
	char Name[MAX_NICKNAME_LEN + 1];
	memcpy(Name, S2CRcvPacket->NickName, MAX_NICKNAME_LEN + 1);
	str += Name;

	m_vector_msg.push_back(str);
	m_log_msg.push_back(str);

	return true;
}

bool ClientProgram::on_packet_proc_leave_player(woodnet::TCPSocket* RecvSock, void* pS2CPacket, int C2SPacketLen)
{
	MSG_S2C_LEAVE_PLAYER* S2CRcvPacket = static_cast<MSG_S2C_LEAVE_PLAYER*>(pS2CPacket);

	std::string str = u8"클라이언트가 나갔습니다. NetID : ";
	INT32 id = S2CRcvPacket->NetID;
	str += std::to_string(id);

	str += " Name : ";
	char Name[MAX_NICKNAME_LEN + 1];
	memcpy(Name, S2CRcvPacket->NickName, MAX_NICKNAME_LEN + 1);
	str += Name;

	m_vector_msg.push_back(str);
	m_log_msg.push_back(str);

	return true;
}

bool ClientProgram::on_packet_proc_change_name(woodnet::TCPSocket* RecvSock, void* pS2CPacket, int C2SPacketLen)
{
	MSG_S2C_CHANGE_NAME_OK* S2CRcvPacket = static_cast<MSG_S2C_CHANGE_NAME_OK*>(pS2CPacket);

	std::string str = u8"클라이언트가 이름을 변경. NetID : ";
	INT32 id = S2CRcvPacket->NetID;
	str += std::to_string(id);

	str += " Name : ";
	char Name[MAX_NICKNAME_LEN + 1];
	memcpy(Name, S2CRcvPacket->NickName, MAX_NICKNAME_LEN + 1);
	str += Name;
	
	m_vector_msg.push_back(str);
	m_log_msg.push_back(str);

	return true;
}

bool ClientProgram::on_packet_proc_chat(woodnet::TCPSocket* RecvSock, void* pS2CPacket, int C2SPacketLen)
{
	MSG_S2C_CHAT* S2CRcvPacket = static_cast<MSG_S2C_CHAT*>(pS2CPacket);

	std::string str = "NetID : ";
	INT32 id = S2CRcvPacket->NetID;
	str += std::to_string(id);

	str += " Name : ";
	char Name[MAX_NICKNAME_LEN + 1];
	memcpy(Name, S2CRcvPacket->NickName, MAX_NICKNAME_LEN + 1);
	str += Name;

	str += " Chat : ";
	char chat[MAX_CHATTING_LEN + 1];
	memcpy(chat, S2CRcvPacket->MSG, MAX_CHATTING_LEN + 1);
	str += chat;

	m_vector_msg.push_back(str);
	m_log_msg.push_back(str);

	return true;
}

bool ClientProgram::on_packet_proc_notice(woodnet::TCPSocket* RecvSock, void* pS2CPacket, int C2SPacketLen)
{
	MSG_S2C_NOTICE* S2CRcvPacket = static_cast<MSG_S2C_NOTICE*>(pS2CPacket);

	std::string str = "<<< 공지 !!! NetID : ";
	INT32 id = S2CRcvPacket->NetID;
	str += std::to_string(id);

	str += " Name : ";
	char Name[MAX_NICKNAME_LEN + 1];
	memcpy(Name, S2CRcvPacket->NickName, MAX_NICKNAME_LEN + 1);
	str += Name;

	str += " Chat : ";
	char chat[MAX_CHATTING_LEN + 1];
	memcpy(chat, S2CRcvPacket->NickName, MAX_CHATTING_LEN + 1);
	str += chat;

	str += " >>>";
	
	m_vector_msg.push_back(str);
	m_log_msg.push_back(str);

	return true;
}