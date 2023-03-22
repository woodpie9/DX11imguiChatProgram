#pragma once
#include <WinSock2.h>
#include <ws2ipdef.h>

#include "CommonDefines.h"

// 20230317 코드 작성 완료
// 20230313 woodpie9 코드 생성 헤더파일작성
// TCP와 UDP의 공유부분을 묘사하는 클래스
// 두 프로토콜이 같이 사용하는 건 bind랑 주소뿐인가...?


WOODNET_BEGIN

struct SOCKET_INFO
{
	SOCKET s;
	SOCKADDR_IN addr;
};


class WinSocket
{
public:
	WinSocket()
	{

	}

	~WinSocket()
	{
		if (m_socket_ != INVALID_SOCKET) closesocket(m_socket_);
		if (m_hEvent_ != WSA_INVALID_EVENT) WSACloseEvent(m_hEvent_);
	}


	SOCKET GetHandle() const;
	bool IsValid() const;

	HANDLE GetEventHandle() const;

	void Attach(SOCKET s);				// 소켓을 연결합니다.
	SOCKET Detach();					// 소켓 연결을 제거합니다.

	bool EventSelect(long lNetworkEvents);

	bool Open(int iProtocol);			// 특정 전송 프토로콜에 맞추어 바인딩 된 소켓을 만듭니다.
	bool Bind(SOCKADDR_IN& localAddr);	// 소켓에 로컬주소를 연결합니다.

	void Close();
	void Shuthdown(int how = SD_BOTH);	// 소켓에서 송신 또는 수신을 사용하지 않도록 설정합니다.

protected:

	SOCKET m_socket_ = INVALID_SOCKET;
	HANDLE m_hEvent_ = WSA_INVALID_EVENT;
	SOCKADDR_IN m_local_addr_ = { 0, };


private:

	WinSocket(const WinSocket&) = delete;
	WinSocket& operator=(const WinSocket&) {}
	WinSocket(WinSocket&&) = delete;

};

WOODNET_END