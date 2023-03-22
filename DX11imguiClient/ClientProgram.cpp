#include "ClientProgram.h"


ClientProgram::ClientProgram()
{
}

bool ClientProgram::init()
{
	return false;
}

bool ClientProgram::connect_server(std::string ConnectIP)
{
}

bool ClientProgram::login_server()
{
	return false;
}

bool ClientProgram::set_nickname(std::string nickname)
{
	return false;
}

bool ClientProgram::change_name(std::string nickname)
{
	return false;
}

bool ClientProgram::network_update()
{
	return false;
}

bool ClientProgram::clean_up()
{
	return false;
}

bool ClientProgram::close()
{
	return false;
}

bool ClientProgram::send_chat_msg(std::string msg)
{
	return false;
}

bool ClientProgram::clear_msg_vector()
{
	return false;
}

bool ClientProgram::send(int PacketID, void* pPacket, int PacketLen)
{
	return false;
}

void ClientProgram::send_flush()
{
}



// 네트워크 이벤트 관리
bool ClientProgram::on_net_connect(woodnet::TCPSocket* pThisSocket)
{
	return false;
}

bool ClientProgram::on_net_close(woodnet::TCPSocket* pThisSocket)
{
	return false;
}

bool ClientProgram::on_net_recv(woodnet::TCPSocket* pThisSocket)
{
	return false;
}

bool ClientProgram::on_net_send(woodnet::TCPSocket* pThisSocket)
{
	return false;
}




// 패킷 이벤트 관리
bool ClientProgram::PacketDispatcher(woodnet::TCPSocket* RecvSock, void* pPacket, int len)
{
	return false;
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

bool ClientProgram::on_packet_proc_s2c_new_player(woodnet::TCPSocket* RecvSock, void* pS2CPacket, int C2SPacketLen)
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
