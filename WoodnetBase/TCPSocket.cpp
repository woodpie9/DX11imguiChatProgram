#include "TCPSocket.h"

woodnet::TCPSocket::TCPSocket()
{
	//HAVE TO DO: 네트워크 설정 관련해서는 별도의 설정 파일로 관리하는 것이 좋습니다.
	const int sendBufSize = 4192;
	const int recvBufSize = 4192;

	m_NetID = -1;

	m_send_buf_.count = 0;
	m_send_buf_.done = 0;
	m_send_buf_.bufSize = sendBufSize;
	m_send_buf_.pBuffer = static_cast<char*>(malloc(sendBufSize));

	m_recv_buf_.count = 0;
	m_recv_buf_.done = 0;
	m_recv_buf_.bufSize = recvBufSize;
	m_recv_buf_.pBuffer = static_cast<char*>(malloc(recvBufSize));

	m_pRecvQ = new StreamQueue(sendBufSize * 4);
	m_pSendQ = new StreamQueue(recvBufSize * 4);
}

woodnet::TCPSocket::~TCPSocket()
{
	free(m_send_buf_.pBuffer);
	free(m_recv_buf_.pBuffer);

	delete m_pRecvQ;
	delete m_pSendQ;
}

bool woodnet::TCPSocket::Connect(SOCKADDR_IN& remoteAddr) const
{
	// 연결되지 않은 소켓, 연결할 주소를 지정, 구조체의 길이, ...
	if (WSAConnect(m_socket_, (SOCKADDR*)(&remoteAddr), sizeof(SOCKADDR_IN), NULL,
		NULL, NULL, NULL) == SOCKET_ERROR)
	{
		// 연결 지향 비 블로킹 소켓의 경우 연결을 즉시 완료할 수 없는 경우가 많습니다.
		// 이러한 경우에는 WSAEWOULDBLOCK을 반환한다. 작업이 진행중이라는 의미입니다.
		if (WSAEWOULDBLOCK != WSAGetLastError()) return false;
	}

	return true;
}

bool woodnet::TCPSocket::Listen()
{
	if (::listen(m_socket_, SOMAXCONN) == SOCKET_ERROR) return false;

	return true;
}

int woodnet::TCPSocket::Send(char* pOutBuf, int nLen, int& error, LPWSAOVERLAPPED pOv, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine) const
{
	WSABUF wsabuf;		// Windock 함수에서 사용하는 데이터 버퍼
	wsabuf.buf = pOutBuf;
	wsabuf.len = nLen;


	DWORD nSent = 0, flag = 0;

	if (WSASend(m_socket_, &wsabuf, 1, &nSent, flag, pOv, lpCompletionRoutine) != SOCKET_ERROR)
	{
		return nSent;
	}

	int err = WSAGetLastError();
	error = err;

	if (WSAEWOULDBLOCK == err || WSA_IO_PENDING == err) return 0;

	return -1;
}

int woodnet::TCPSocket::Recv(char* pInBuf, int nLen, int& error, LPWSAOVERLAPPED pOv, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	WSABUF wsabuf;		// Windock 함수에서 사용하는 데이터 버퍼
	wsabuf.buf = pInBuf;
	wsabuf.len = nLen;

	DWORD nRecv = 0, flag = 0;

	// 연결된 소켓, WSABUF 구조의 배열에 대한 포인터, 구조의 수, 수신작업 완료시 수신된 데이터의 바이트 수를 담는 포인터, ... 
	if (WSARecv(m_socket_, &wsabuf, 1, &nRecv,
		&flag, (LPWSAOVERLAPPED)pOv, lpCompletionRoutine) != SOCKET_ERROR)
	{
		return nRecv;
	}

	int err = WSAGetLastError();
	error = err;

	if (WSAEWOULDBLOCK == err || WSA_IO_PENDING == err) return 0;

	return -1;
}

bool woodnet::TCPSocket::SendUpdate()
{
	int error = 0;
	int nSent = 0;
	char* pStart = nullptr;
	int remain = m_send_buf_.count - m_send_buf_.done;

	if (remain > 0)
	{
		// 송신 버퍼가 비어있지 않다면 남은 데이터를 마저 보냅니다.
		pStart = &m_send_buf_.pBuffer[m_send_buf_.done];
	}
	else if (remain == 0)
	{
		// 송신 버퍼의 데이터를 다 보냈다면 스트링큐에서 데이터를 가져옵니다.
		m_send_buf_.count = m_pSendQ->read(m_send_buf_.pBuffer, m_send_buf_.bufSize);
		m_send_buf_.done = 0;

		remain = m_send_buf_.count;
		pStart = m_send_buf_.pBuffer;
	}

	// 0보다 작은 값이 들어오면 작업 중지
	assert(remain >= 0);

	nSent = Send(pStart, remain, error);
	if (nSent < 0) return false;

	m_send_buf_.done += nSent;

	return true;
}

bool woodnet::TCPSocket::RecvUpdate()
{
	int error = 0;

	// 소켓 버퍼의 데이터를 가져옵니다.
	int space = m_recv_buf_.bufSize - m_recv_buf_.count;
	if (space > 0)
	{
		int nRecv = Recv(&m_recv_buf_.pBuffer[m_recv_buf_.count], space, error);

		if (nRecv > 0)
		{
			m_recv_buf_.count += nRecv;
		}
	}

	// 수신 버퍼의 데이터를 스트림 큐로 보냅니다.
	int remain = m_recv_buf_.count - m_recv_buf_.done;

	// 0보다 작은 값이 들어오면 작업 중지
	assert(remain >= 0);

	// 보낼 데이터가 있다면 보냅니다.
	if (remain > 0)
	{
		m_recv_buf_.done += m_pRecvQ->write(&m_recv_buf_.pBuffer[m_recv_buf_.done], remain);
	}

	// 데이터를 모두 보냈으니 수신 버퍼를 리셋합니다.
	if (m_recv_buf_.done == m_recv_buf_.count)
	{
		m_recv_buf_.count = 0;
		m_recv_buf_.done = 0;
	}

	return true;
}

bool woodnet::TCPSocket::PostPacket(char* buffer, int nSize) const
{
	if (m_pSendQ->remain() < nSize) return false;

	m_pSendQ->write(buffer, nSize);

	return true;
}

bool woodnet::TCPSocket::PeekPacket(char* buffer, int nSize) const
{
	return m_pRecvQ->peek(buffer, nSize);
}

bool woodnet::TCPSocket::ReadPacket(char* buffer, int nSize) const
{
	if (m_pRecvQ->count() < nSize) return false;

	m_pRecvQ->read(buffer, nSize);

	return true;
}

void woodnet::TCPSocket::Reset()
{
	m_send_buf_.count = 0;
	m_send_buf_.done = 0;

	m_recv_buf_.count = 0;
	m_recv_buf_.done = 0;

	m_pSendQ->clear();
	m_pRecvQ->clear();
}

