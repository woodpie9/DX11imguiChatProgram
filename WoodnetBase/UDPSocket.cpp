#include "UDPSocket.h"

int woodnet::UDPSocket::RecvFrom(char* pInBuf, int nLen, int& error, SOCKADDR_IN& remoteAddr, LPWSAOVERLAPPED pOv)
{
	WSABUF	wsabuf;
	wsabuf.buf = pInBuf;
	wsabuf.len = nLen;

	DWORD nRecv = 0, flag = 0;

	int	iAddrLen = sizeof(remoteAddr);

	if (WSARecvFrom(m_socket_, &wsabuf, 1, &nRecv, &flag, (SOCKADDR*)&remoteAddr, &iAddrLen, pOv, NULL) != SOCKET_ERROR)
	{
		return nRecv;
	}

	int err = WSAGetLastError();
	error = err;

	if (WSAEWOULDBLOCK == err) return 0;

	return -1;
}

int woodnet::UDPSocket::SendTo(char* pOutBuf, int nLen, int& error, SOCKADDR_IN& remoteAddr, LPWSAOVERLAPPED pOv)
{
	WSABUF	wsabuf;
	wsabuf.buf = pOutBuf;
	wsabuf.len = nLen;

	DWORD nSent = 0, flag = 0;
	if (WSASendTo(m_socket_, &wsabuf, 1, &nSent, flag, (SOCKADDR*)&remoteAddr, sizeof(remoteAddr), pOv, NULL)!= SOCKET_ERROR)
	{
		return nSent;
	}

	int err = WSAGetLastError();
	error = err;

	if (WSAEWOULDBLOCK == err) return 0;

	return -1;
}
