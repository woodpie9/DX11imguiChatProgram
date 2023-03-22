#include "WinSocket.h"

SOCKET woodnet::WinSocket::GetHandle() const
{
	return m_socket_;
}

bool woodnet::WinSocket::IsValid() const
{
	return ((INVALID_SOCKET == m_socket_) ? false : true);
}


HANDLE woodnet::WinSocket::GetEventHandle() const
{
	return m_hEvent_;
}

void woodnet::WinSocket::Attach(SOCKET s)
{
	m_socket_ = s;
}

SOCKET woodnet::WinSocket::Detach()
{
	SOCKET s = m_socket_;
	m_socket_ = INVALID_SOCKET;
	return s;
}

bool woodnet::WinSocket::EventSelect(long lNetworkEvents)
{
	if (m_hEvent_ == WSA_INVALID_EVENT)
	{
		if ((m_hEvent_ = WSACreateEvent()) == WSA_INVALID_EVENT) return false;
	}

	if (WSAEventSelect(m_socket_, m_hEvent_, lNetworkEvents) == SOCKET_ERROR) return false;

	return true;
}

bool woodnet::WinSocket::Open(int iProtocol)
{
	if (iProtocol == IPPROTO_TCP)
	{
		// IPv4 주소 패밀리 사용, 신뢰할 수 있는 양방향 연결 기반 바이트 스트림을 제공하는 소켓, 사용할 프로토콜, 생성할 소켓의 특성, 소켓 그룹 ID, 추가 속성 지정
		m_socket_ = WSASocket(AF_INET, SOCK_STREAM, iProtocol, NULL, NULL, WSA_FLAG_OVERLAPPED);
	}
	else if (IPPROTO_UDP)
	{
		// IPv4주소 패밀리 사용, 연결이 없고 고정된 센뢰할 수 없는 버퍼인 데이터그램을 지원하는 소켓, ... 
		m_socket_ = WSASocket(AF_INET, SOCK_DGRAM, iProtocol, NULL, NULL, WSA_FLAG_OVERLAPPED);
	}

	if (!IsValid()) return false;

	return true;
}

bool woodnet::WinSocket::Bind(SOCKADDR_IN& localAddr)
{
	if (::bind(m_socket_, (SOCKADDR*)(&localAddr), sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
		return false;

	m_local_addr_ = localAddr;

	return true;
}

void woodnet::WinSocket::Close()
{
	::closesocket(m_socket_);
	m_socket_ = INVALID_SOCKET;
}

void woodnet::WinSocket::Shuthdown(int how)
{
	::shutdown(m_socket_, how);
}
