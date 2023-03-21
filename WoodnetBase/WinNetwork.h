#pragma once
#include "WinSocket.h"

// 20230317 추가 분석, 주석 작성
// 20230316 코드 생성
// Windows Socket을 사용하기 위한 초기화와 종                           

WOODNET_BEGIN

class WinNetwork
{
public:
	WinNetwork(const WinNetwork&) = delete;
	WinNetwork(WinNetwork&&) = delete;
	WinNetwork& operator=(const WinNetwork&) = delete;

	WinNetwork() {}
	~WinNetwork() {}

	bool Init();
	void CleanUp();

private:
	WSADATA m_wsa_data_ = { 0, };		// Windows 소켓 구현에 대한 정보를 담기 위한 구조체
};

WOODNET_END