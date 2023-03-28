#include "UDPSocket.h"

int woodnet::UDPSocket::RecvFrom(char* in_buf, int len, int& error, SOCKADDR_IN& remote_addr, LPWSAOVERLAPPED ov)
{
	WSABUF	wsabuf;
	wsabuf.buf = in_buf;
	wsabuf.len = len;

	DWORD nRecv = 0, flag = 0;

	int	iAddrLen = sizeof(remote_addr);

	if (WSARecvFrom(m_socket_, &wsabuf, 1, &nRecv, &flag, (SOCKADDR*)&remote_addr, &iAddrLen, ov, NULL) != SOCKET_ERROR)
	{
		return nRecv;
	}

	int err = WSAGetLastError();
	error = err;

	if (WSAEWOULDBLOCK == err) return 0;

	return -1;
}

int woodnet::UDPSocket::SendTo(char* out_buf, int len, int& error, SOCKADDR_IN& remote_addr, LPWSAOVERLAPPED ov)
{
	WSABUF	wsabuf;
	wsabuf.buf = out_buf;
	wsabuf.len = len;

	DWORD nSent = 0, flag = 0;
	if (WSASendTo(m_socket_, &wsabuf, 1, &nSent, flag, (SOCKADDR*)&remote_addr, sizeof(remote_addr), ov, NULL)!= SOCKET_ERROR)
	{
		return nSent;
	}

	int err = WSAGetLastError();
	error = err;

	if (WSAEWOULDBLOCK == err) return 0;

	return -1;
}
