#include "TCPSocket.h"

woodnet::TCPSocket::TCPSocket()
{
	//HAVE TO DO: 네트워크 설정 관련해서는 별도의 설정 파일로 관리하는 것이 좋습니다.
	const int send_buf_size = 4192;
	const int recv_buf_size = 4192;

	net_id_ = -1;

	send_buf_.count = 0;
	send_buf_.done = 0;
	send_buf_.buf_size = send_buf_size;
	send_buf_.p_buffer = static_cast<char*>(malloc(send_buf_size));

	recv_buf_.count = 0;
	recv_buf_.done = 0;
	recv_buf_.buf_size = recv_buf_size;
	recv_buf_.p_buffer = static_cast<char*>(malloc(recv_buf_size));

	recv_q_ = new StreamQueue(send_buf_size * 4);
	send_q_ = new StreamQueue(recv_buf_size * 4);
}

woodnet::TCPSocket::~TCPSocket()
{
	free(send_buf_.p_buffer);
	free(recv_buf_.p_buffer);

	delete recv_q_;
	delete send_q_;
}

bool woodnet::TCPSocket::Connect(SOCKADDR_IN& remote_addr) const
{
	// 연결되지 않은 소켓, 연결할 주소를 지정, 구조체의 길이, ...
	if (WSAConnect(m_socket_, (SOCKADDR*)(&remote_addr), sizeof(SOCKADDR_IN),
		NULL, NULL, NULL, NULL) == SOCKET_ERROR)
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

int woodnet::TCPSocket::Send(char* p_out_buf, int len, int& error, LPWSAOVERLAPPED p_ov,
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lp_completion_routine) const
{
	WSABUF wsabuf{};		// Windock 함수에서 사용하는 데이터 버퍼
	wsabuf.buf = p_out_buf;
	wsabuf.len = len;


	DWORD sent = 0, flag = 0;

	if (WSASend(m_socket_, &wsabuf, 1, &sent, flag, p_ov, 
		lp_completion_routine) != SOCKET_ERROR)
	{
		return sent;
	}

	int err = WSAGetLastError();
	error = err;

	if (WSAEWOULDBLOCK == err || WSA_IO_PENDING == err) return 0;

	return -1;
}

int woodnet::TCPSocket::Recv(char* p_in_buf, int len, int& error, LPWSAOVERLAPPED p_ov, LPWSAOVERLAPPED_COMPLETION_ROUTINE lp_completion_routine)
{
	WSABUF wsabuf;		// Windock 함수에서 사용하는 데이터 버퍼
	wsabuf.buf = p_in_buf;
	wsabuf.len = len;

	DWORD recv = 0, flag = 0;

	// 연결된 소켓, WSABUF 구조의 배열에 대한 포인터, 구조의 수, 수신작업 완료시 수신된 데이터의 바이트 수를 담는 포인터, ... 
	if (WSARecv(m_socket_, &wsabuf, 1, &recv,
		&flag, (LPWSAOVERLAPPED)p_ov, lp_completion_routine) != SOCKET_ERROR)
	{
		return recv;
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
	int remain = send_buf_.count - send_buf_.done;

	if (remain > 0)
	{
		// 송신 버퍼가 비어있지 않다면 남은 데이터를 마저 보냅니다.
		pStart = &send_buf_.p_buffer[send_buf_.done];
	}
	else if (remain == 0)
	{
		// 송신 버퍼의 데이터를 다 보냈다면 스트링큐에서 데이터를 가져옵니다.
		send_buf_.count = send_q_->Read(send_buf_.p_buffer, send_buf_.buf_size);
		send_buf_.done = 0;

		remain = send_buf_.count;
		pStart = send_buf_.p_buffer;
	}

	// 0보다 작은 값이 들어오면 작업 중지
	assert(remain >= 0);

	nSent = Send(pStart, remain, error);
	if (nSent < 0) return false;

	send_buf_.done += nSent;

	return true;
}

bool woodnet::TCPSocket::RecvUpdate()
{
	int error = 0;

	// 소켓 버퍼의 데이터를 가져옵니다.
	int space = recv_buf_.buf_size - recv_buf_.count;
	if (space > 0)
	{
		int nRecv = Recv(&recv_buf_.p_buffer[recv_buf_.count], space, error);

		if (nRecv > 0)
		{
			recv_buf_.count += nRecv;
		}
	}

	// 수신 버퍼의 데이터를 스트림 큐로 보냅니다.
	int remain = recv_buf_.count - recv_buf_.done;

	// 0보다 작은 값이 들어오면 작업 중지
	assert(remain >= 0);

	// 보낼 데이터가 있다면 보냅니다.
	if (remain > 0)
	{
		recv_buf_.done += recv_q_->Write(&recv_buf_.p_buffer[recv_buf_.done], remain);
	}

	// 데이터를 모두 보냈으니 수신 버퍼를 리셋합니다.
	if (recv_buf_.done == recv_buf_.count)
	{
		recv_buf_.count = 0;
		recv_buf_.done = 0;
	}

	return true;
}

bool woodnet::TCPSocket::PostPacket(char* buffer, int size) const
{
	if (send_q_->Remain() < size) return false;

	send_q_->Write(buffer, size);

	return true;
}

bool woodnet::TCPSocket::PeekPacket(char* buffer, int size) const
{
	return recv_q_->Peek(buffer, size);
}

bool woodnet::TCPSocket::ReadPacket(char* buffer, int size) const
{
	if (recv_q_->Count() < size) return false;

	recv_q_->Read(buffer, size);

	return true;
}

void woodnet::TCPSocket::Reset()
{
	send_buf_.count = 0;
	send_buf_.done = 0;

	recv_buf_.count = 0;
	recv_buf_.done = 0;

	send_q_->Clear();
	recv_q_->Clear();
}

