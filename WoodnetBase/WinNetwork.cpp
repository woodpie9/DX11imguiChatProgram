#include "WinNetwork.h"

// Windows Socket을 사용하기 위한 라이브러리
#pragma comment(lib, "Ws2_32.lib")

WOODNET_BEGIN

// Windows 소켓 함수 중 첫 번쨰로 호출해야하는 함수
// windows 소켓 버전을 지정하고 소켓 구현의 세부 정보를 넣는다
// 어플리케이션 또는 DLL은 WSAStartup을 성공적으로 호출한 후에만 소켓 함수를 실행 가능
bool WinNetwork::Init()
{
	// Windows 소켓 사양을 2.2로 지정한다.
	// Windows 소켓 수현의 세부 정보를 수신하기 위하여 WSADATA 구조체의 포인터를 넘긴다.
	if (!WSAStartup(MAKEWORD(2, 2), &m_wsa_data_))
	{
		return false;
	}

	return true;
}

void WinNetwork::CleanUp()                           
{
	// Winsock2의 사용을 종료한다.
	WSACleanup();
}

WOODNET_END