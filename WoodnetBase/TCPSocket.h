#pragma once
#include <assert.h>
#include "WinSocket.h"
#include "StreamQueue.h"

// 20230314 woodpie9 코드 생성 
// WinSocket을 상속 받는 TCP 소켓을 묘사하는 클래스


WOODNET_BEGIN

class TCPSocket : public WinSocket
{
	struct TCPBuffer
	{
		short count;		// 데이터의 총 갯수
		short done;			// 보낸 데이터의 수
		short bufSize;
		char* pBuffer;
	};

public:
	TCPSocket(const TCPSocket&) = delete;
	TCPSocket& operator=(const TCPSocket&) = delete;

	TCPSocket();
	~TCPSocket();

	// 클라이언트가 서버에 연결하기 위해 IP주소와 포트번호 연동
	bool Connect(SOCKADDR_IN& remoteAddr) const;		
	// 서버가 들어오는 연결을 듣기 위하여 소켓을 배치합니다.
	bool Listen();

	// 연결된 소켓에 데이터를 보냅니다.
	int	Send(char* pOutBuf, int nLen, int& error, LPWSAOVERLAPPED pOv = NULL, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine = NULL) const;
	// 연결된 소켓 또는 바인딩된 연결 없는 소켓에서 데이터를 받습니다.
	int	Recv(char* pInBuf, int nLen, int& error, LPWSAOVERLAPPED pOv = NULL, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine = NULL);


	bool SendUpdate();		// 데이터를 
	bool RecvUpdate();

	bool PostPacket(char* buffer, int nSize) const;		// 데이터를 큐에 씁니다.
	bool PeekPacket(char* buffer, int nSize) const;		// 데이터를 큐에서 지우지 않고 읽습니다.
	bool ReadPacket(char* buffer, int nSize) const;		// 데이터를 큐에서 읽습니다.

	void Reset();


	void SetNetID(NetworkObjectID ID);
	NetworkObjectID GetNetID();

protected:
	NetworkObjectID m_NetID;

private:
	TCPBuffer m_send_buf_;		// 송신버퍼. 송신할 데이터를 저장하는 버퍼
	TCPBuffer m_recv_buf_;		// 수신버퍼. 수신한 데이터를 저장하는 버퍼

	StreamQueue* m_pSendQ;		// 송신버퍼. 프로그램에서 송신할 데이터를 저장하는 스트림 큐
	StreamQueue* m_pRecvQ;		// 수신버퍼. 프로그램이 수신할 데이터를 저장하는 스트림 큐
};


WOODNET_END