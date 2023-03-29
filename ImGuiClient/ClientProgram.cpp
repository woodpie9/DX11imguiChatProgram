
#include "ClientProgram.h"
#include "../WoodnetBase/WoodnetProtocol.h"



static constexpr int PACKET_HEADER_SIZE = sizeof(PACKET_HEADER);

ClientProgram::ClientProgram() : client_(false), connect_(false)
{
	set_last_error(_Error::success);
}

bool ClientProgram::Init()
{
	log_msg_.emplace_back("init client...");

	// 이미 init 완료함
	if (client_)
	{
		log_msg_.emplace_back("already_init");
		set_last_error(_Error::already_init);
		return false;
	}

	// 이미 연결 되어있음
	if (connect_)
	{
		log_msg_.emplace_back("already_connect");
		set_last_error(_Error::already_connect);
		return false;
	}

	// 소켓을 연다.
	if (connector_.Open(IPPROTO_TCP) == false)
	{
		log_msg_.emplace_back("socket open fail");
		set_last_error(_Error::fatal);
		connect_ = false;
		return false;
	}
	set_connection_status(ConnectionStatus::Oppend);

	// 소켓에 내가 관심있는 이벤트를 지정한다.
	if (connector_.EventSelect(FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE) == false)
	{
		log_msg_.emplace_back("eventselect fail");
		set_last_error(_Error::fatal);
		connect_ = false;
		return false;
	}
	set_connection_status(ConnectionStatus::SetEvent);


	client_ = true;

	set_last_error(_Error::success);
	return  true;
}

bool ClientProgram::ConnectServer(std::string connect_ip)
{
	bool result = false;
	log_msg_.emplace_back("connecting server...");

	// Init 필요
	if (!client_)
	{
		log_msg_.emplace_back("not_init");
		set_last_error(_Error::not_init);
		return false;
	}

	// 이미 연결 되어있음
	if (connect_)
	{
		log_msg_.emplace_back("already_connect");
		set_last_error(_Error::already_connect);
		return false;
	}

	// 소켓에 정보를 바인딩 한다.
	SOCKADDR_IN connect_addr;
	connect_addr.sin_family = AF_INET;
	InetPtonA(AF_INET, connect_ip.c_str(), &connect_addr.sin_addr);
	connect_addr.sin_port = htons(port_);

	// 소켓의 정보로 서버에 연결한다.
	if (connector_.Connect(connect_addr) == false)
	{
		log_msg_.emplace_back("connect fail");
		set_last_error(_Error::fatal);
		connect_ = false;
		return false;
	}

	// 이벤트를 등록한다.
	event_ = connector_.GetEventHandle();

	log_msg_.push_back("Connect to " + connect_ip + ":" + std::to_string(port_));
	set_connection_status(ConnectionStatus::Connecting);

	connect_ = true;
	return true;
}

bool ClientProgram::LoginServer()
{
	if (CheckConnect() == false) return false;

	log_msg_.emplace_back("Login server...");

	static MSG_C2S_ENTER_LOBBY C2SSendPacket;

	C2SSendPacket.packet_size = sizeof(MSG_C2S_ENTER_LOBBY);
	C2SSendPacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::C2S_ENTER_LOBBY);
	C2SSendPacket.seqNum = 1;

	connector_.PostPacket(reinterpret_cast<char*>(&C2SSendPacket), C2SSendPacket.packet_size);

	return true;
}

bool ClientProgram::SetNickname(std::string nickname)
{
	if (CheckConnect() == false) return false;

	log_msg_.emplace_back("Set Nickname...");

	static MSG_C2S_ENTER_CHAT_SER C2SSendPacket;

	C2SSendPacket.packet_size = sizeof(MSG_C2S_ENTER_CHAT_SER);
	C2SSendPacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::C2S_ENTER_CHAT_SER);
	C2SSendPacket.seqNum = 2;

	const char* cstr = nickname.c_str();
	memcpy(C2SSendPacket.NickName, cstr, MAX_NICKNAME_LEN + 1);

	connector_.PostPacket((char*)&C2SSendPacket, C2SSendPacket.packet_size);

	return true;
}

bool ClientProgram::ChangeName(std::string nickname)
{
	if (CheckConnect() == false) return false;

	log_msg_.emplace_back("Change Nickname...");

	static MSG_C2S_CHANGE_NAME C2SSendPacket;

	C2SSendPacket.packet_size = sizeof(MSG_C2S_CHANGE_NAME);
	C2SSendPacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::C2S_CHANGE_NAME);
	C2SSendPacket.seqNum = 3;

	const char* cstr = nickname.c_str();
	memcpy(C2SSendPacket.NickName, cstr, MAX_NICKNAME_LEN + 1);

	connector_.PostPacket(reinterpret_cast<char*>(&C2SSendPacket), C2SSendPacket.packet_size);

	return true;
}

bool ClientProgram::NetworkUpdate()
{
	if (CheckConnect() == false) return false;

	// 새로 들어온 이벤트가 있는지 확인한다.
	static int EventReturn = 0;
	if ((EventReturn = WSAWaitForMultipleEvents(1, &event_, FALSE, 1, FALSE)) == WSA_WAIT_FAILED)
	{
		log_msg_.push_back("WSAWaitForMultipleEvents() failed with error " 
			+ std::to_string(WSAGetLastError()));
		set_last_error(_Error::fatal);
		return false;
	}

	if (EventReturn == WSA_WAIT_TIMEOUT)
	{
		// 타임아웃이면 통제권을 넘겨준다.
		// SendFlush를 TimeOut때 할까?
		SendFlush();
		set_last_error(_Error::success);
		return true;
	}

	// 이번에 받은 이벤트가 어떤건지 확인한다.
	WSANETWORKEVENTS NetworkEvents;
	if (WSAEnumNetworkEvents(connector_.GetHandle(), connector_.GetEventHandle(), &NetworkEvents) == SOCKET_ERROR)
	{
		log_msg_.push_back("WSAEnumNetworkEvents() failed with error " 
			+ std::to_string(WSAGetLastError()));
		set_last_error(_Error::error);
		return false;
	}


	if (NetworkEvents.lNetworkEvents & FD_CONNECT)
	{
		if (NetworkEvents.iErrorCode[FD_CONNECT_BIT] != 0)
		{
			log_msg_.push_back("FD_CONNECT failed with error " 
				+ std::to_string(NetworkEvents.iErrorCode[FD_CONNECT_BIT]));
			set_last_error(_Error::error);
			return false;
		}

		OnNetConnect(&connector_);
	}


	if (NetworkEvents.lNetworkEvents & FD_CLOSE)
	{
		if (NetworkEvents.iErrorCode[FD_CLOSE_BIT] != 0)
		{
			log_msg_.push_back("FD_CONNECT failed with error " 
				+ std::to_string(NetworkEvents.iErrorCode[FD_CONNECT_BIT]));
			set_last_error(_Error::error);
			return false;
		}

		OnNetConnect(&connector_);
	}


	if (NetworkEvents.lNetworkEvents & FD_READ)
	{
		if (NetworkEvents.iErrorCode[FD_READ_BIT] != 0)
		{
			log_msg_.push_back("FD_CONNECT failed with error "
				+ std::to_string(NetworkEvents.iErrorCode[FD_CONNECT_BIT]));
			set_last_error(_Error::error);
			return false;
		}

		OnNetRecv(&connector_);
	}


	if (NetworkEvents.lNetworkEvents & FD_WRITE)
	{
		if (NetworkEvents.iErrorCode[FD_WRITE_BIT] != 0)
		{
			log_msg_.push_back("FD_CONNECT failed with error " 
				+ std::to_string(NetworkEvents.iErrorCode[FD_CONNECT_BIT]));
			set_last_error(_Error::error);
			return false;
		}

		OnNetConnect(&connector_);
	}

	return true;
}

//bool ClientProgram::CleanUp()
//{
//	if (check_connect() == false) return false;
//
//	m_connector.Close();
//
//	return false;
//}

bool ClientProgram::Close()
{
	if (CheckConnect() == false) return false;

	connector_.Close();

	return true;
}

bool ClientProgram::SendChatMsg(std::string msg)
{
	if (CheckConnect() == false) return false;

	static MSG_C2S_CHAT C2SSendPacket;

	C2SSendPacket.packet_size = sizeof(MSG_C2S_CHAT);
	C2SSendPacket.packet_id = static_cast<INT16>(LOBBY_PACKET_ID::C2S_CHAT);
	C2SSendPacket.seqNum = 1;

	const char* cMSG = msg.c_str();
	memcpy(C2SSendPacket.MSG, cMSG, MAX_CHATTING_LEN + 1);

	connector_.PostPacket(reinterpret_cast<char*>(&C2SSendPacket), C2SSendPacket.packet_size);


	return true;
}

bool ClientProgram::CheckConnect()
{
	// Init 필요
	if (!client_)
	{
		log_msg_.emplace_back("not_init");
		set_last_error(_Error::not_init);
		return false;
	}

	// 이미 연결 되어있음
	if (!connect_)
	{
		log_msg_.emplace_back("not_connect");
		set_last_error(_Error::not_connect);
		return false;
	}

	return true;
}

bool ClientProgram::Send(int packet_id, void* p_packet, int packet_len)
{
	if (CheckConnect() == false) return false;

	if (connector_.PostPacket(static_cast<char*>(p_packet), packet_len) == false)
	{
		log_msg_.emplace_back("PostPacket Fail");
		set_last_error(_Error::error);
		return false;
	}

	return true;
}

void ClientProgram::SendFlush()
{
	connector_.SendUpdate();
}



// 네트워크 이벤트 관리
bool ClientProgram::OnNetConnect(woodnet::TCPSocket* p_this_socket)
{
	log_msg_.emplace_back("Connect is OK!");
	set_connection_status(ConnectionStatus::Connected);
	set_last_error(_Error::success);
	return true;
}

bool ClientProgram::OnNetClose(woodnet::TCPSocket* p_this_socket)
{
	log_msg_.emplace_back("close packet");
	set_connection_status(ConnectionStatus::Closed);
	set_last_error(_Error::success);
	return true;
}

bool ClientProgram::OnNetRecv(woodnet::TCPSocket* p_this_socket)
{
	static PACKET_HEADER msg_header;
	connector_.RecvUpdate();

	while (connector_.PeekPacket(reinterpret_cast<char*>(&msg_header), PACKET_HEADER_SIZE))
	{
		// 온전한 패킷이 있다면 패킷을 만들어서 처리.
		memset(&packet_recv_buf_, 0, PACKET_SIZE_MAX);

		if (msg_header.packet_size > PACKET_SIZE_MAX || msg_header.packet_size < 0)
		{
			log_msg_.push_back("OnNetRecv packet size error " + std::to_string(msg_header.packet_size));
			set_last_error(_Error::error);
			continue;
		}

		// 크기가 알맞는 하나의 패킷을 읽어온다.
		if (connector_.ReadPacket(packet_recv_buf_, msg_header.packet_size))
		{
			// 패킷을 해석하고 알맞는 동작을 한다.
			PacketDispatcher(p_this_socket, packet_recv_buf_, msg_header.packet_size);
		}
		else
		{
			log_msg_.emplace_back("Connector.ReadPacket return false!");
			break;
		}
	}

	return true;
}

bool ClientProgram::OnNetSend(woodnet::TCPSocket* pThisSocket)
{
	//TODO : 송신 가능한 상태 관리에 사용. 여기선 미사용
	return true;
}




// 패킷 이벤트 관리
bool ClientProgram::PacketDispatcher(woodnet::TCPSocket* recv_sock, void* p_packet, int len)
{
	PACKET_HEADER* p_header = (PACKET_HEADER*)p_packet;

	switch (p_header->packet_id)
	{
		case static_cast<INT16>(COMMON_PAKET_ID::S2C_ACCEPT):
		{
			return OnPacketProcAccept(recv_sock, p_packet, len);
		}
		break;

		case static_cast<INT16>(LOBBY_PACKET_ID::S2C_ENTER_LOBBY_OK):
		{
			return OnPacketProcEnterLobby(recv_sock, p_packet, len);
		}
		break;

		case static_cast<INT16>(LOBBY_PACKET_ID::S2C_ENTER_CHAT_SER_OK):
		{
			return OnPacketProcEnterChat(recv_sock, p_packet, len);
		}
		break;

		case static_cast<INT16>(LOBBY_PACKET_ID::S2C_NEW_PLAYER):
		{
			return OnPacketProcNewPlayer(recv_sock, p_packet, len);
		}
		break;

		case static_cast<INT16>(LOBBY_PACKET_ID::S2C_LEAVE_PLAYER):
		{
			return OnPacketProcLeavePlayer(recv_sock, p_packet, len);
		}
		break;

		case static_cast<INT16>(LOBBY_PACKET_ID::S2C_CHANGE_NAME_OK):
		{
			return OnPacketProcChangeName(recv_sock, p_packet, len);
		}
		break;

		case static_cast<INT16>(LOBBY_PACKET_ID::S2C_CHAT):
		{
			return OnPacketProcChat(recv_sock, p_packet, len);
		}
		break;

		case static_cast<INT16>(LOBBY_PACKET_ID::S2C_NOTICE):
		{
			return OnPacketProcNotice(recv_sock, p_packet, len);
		}
		break;

		default:
		{
			log_msg_.push_back("Unknown Packet! id : " + std::to_string(p_header->packet_id) + "size : " + std::to_string(p_header->packet_size));
			set_last_error(_Error::error);
			return false;
		}
		break;
	}
}

bool ClientProgram::OnPacketProcAccept(woodnet::TCPSocket* recv_sock, void* ps2c_packet, int c2s_packet_len)
{
	MSG_S2C_ACCEPT* s2c_rcv_packet = static_cast<MSG_S2C_ACCEPT*>(ps2c_packet);

	my_net_id_ = s2c_rcv_packet->NetID;
	log_msg_.push_back("Enter Server. NetID : " + std::to_string(my_net_id_));

	return true;
}

bool ClientProgram::OnPacketProcEnterLobby(woodnet::TCPSocket* recv_sock, void* ps2c_packet, int c2s_packet_len)
{
	log_msg_.emplace_back("pls set nickname");

	return true;
}

bool ClientProgram::OnPacketProcEnterChat(woodnet::TCPSocket* recv_sock, void* ps2c_packet, int c2s_packet_len)
{
	MSG_S2C_ENTER_CHAT_SER_OK* s2c_rcv_packet = static_cast<MSG_S2C_ENTER_CHAT_SER_OK*>(ps2c_packet);

	bool result = s2c_rcv_packet->Result;

	if (result == true)
	{
		log_msg_.emplace_back("success Into Chat room!!!");
		set_connection_status(ConnectionStatus::OnChat);
		return false;
	}
	else
	{
		log_msg_.emplace_back("fail into chat room...");

		return true;
	}
}

bool ClientProgram::OnPacketProcNewPlayer(woodnet::TCPSocket* recv_sock, void* ps2c_packet, int c2s_packet_len)
{
	MSG_S2C_NEW_PLAYER* s2c_rcv_packet = static_cast<MSG_S2C_NEW_PLAYER*>(ps2c_packet);

	std::string str = u8"새로운 클라이언트 접속. NetID : ";
	INT32 id = s2c_rcv_packet->NetID;
	str += std::to_string(id);

	str += " Name : ";
	char name[MAX_NICKNAME_LEN + 1];
	memcpy(name, s2c_rcv_packet->NickName, MAX_NICKNAME_LEN + 1);
	str += name;

	vector_msg_.push_back(str);
	log_msg_.push_back(str);

	return true;
}

bool ClientProgram::OnPacketProcLeavePlayer(woodnet::TCPSocket* recv_sock, void* ps2c_packet, int c2s_packet_len)
{
	MSG_S2C_LEAVE_PLAYER* s2c_rcv_packet = static_cast<MSG_S2C_LEAVE_PLAYER*>(ps2c_packet);

	std::string str = u8"클라이언트가 나갔습니다. NetID : ";
	INT32 id = s2c_rcv_packet->NetID;
	str += std::to_string(id);

	str += " Name : ";
	char name[MAX_NICKNAME_LEN + 1];
	memcpy(name, s2c_rcv_packet->NickName, MAX_NICKNAME_LEN + 1);
	str += name;

	vector_msg_.push_back(str);
	log_msg_.push_back(str);

	return true;
}

bool ClientProgram::OnPacketProcChangeName(woodnet::TCPSocket* recv_sock, void* ps2c_packet, int c2s_packet_len)
{
	MSG_S2C_CHANGE_NAME_OK* s2c_rcv_packet = static_cast<MSG_S2C_CHANGE_NAME_OK*>(ps2c_packet);

	std::string str = u8"클라이언트가 이름을 변경. NetID : ";
	INT32 id = s2c_rcv_packet->NetID;
	str += std::to_string(id);

	str += " Name : ";
	char name[MAX_NICKNAME_LEN + 1];
	memcpy(name, s2c_rcv_packet->NickName, MAX_NICKNAME_LEN + 1);
	str += name;
	
	vector_msg_.push_back(str);
	log_msg_.push_back(str);

	return true;
}

bool ClientProgram::OnPacketProcChat(woodnet::TCPSocket* recv_sock, void* ps2c_packet, int c2s_packet_len)
{
	MSG_S2C_CHAT* s2c_rcv_packet = static_cast<MSG_S2C_CHAT*>(ps2c_packet);

	std::string str = "NetID : ";
	INT32 id = s2c_rcv_packet->NetID;
	str += std::to_string(id);

	str += " Name : ";
	char name[MAX_NICKNAME_LEN + 1];
	memcpy(name, s2c_rcv_packet->NickName, MAX_NICKNAME_LEN + 1);
	str += name;

	str += " Chat : ";
	char chat[MAX_CHATTING_LEN + 1];
	memcpy(chat, s2c_rcv_packet->MSG, MAX_CHATTING_LEN + 1);
	str += chat;

	vector_msg_.push_back(str);
	log_msg_.push_back(str);

	return true;
}

bool ClientProgram::OnPacketProcNotice(woodnet::TCPSocket* recv_sock, void* ps2c_packet, int c2s_packet_len)
{
	MSG_S2C_NOTICE* s2c_rcv_packet = static_cast<MSG_S2C_NOTICE*>(ps2c_packet);

	std::string str = "<<< 공지 !!! NetID : ";
	INT32 id = s2c_rcv_packet->NetID;
	str += std::to_string(id);

	str += " Name : ";
	char name[MAX_NICKNAME_LEN + 1];
	memcpy(name, s2c_rcv_packet->NickName, MAX_NICKNAME_LEN + 1);
	str += name;

	str += " Chat : ";
	char chat[MAX_CHATTING_LEN + 1];
	memcpy(chat, s2c_rcv_packet->NickName, MAX_CHATTING_LEN + 1);
	str += chat;

	str += " >>>";
	
	vector_msg_.push_back(str);
	log_msg_.push_back(str);

	return true;
}