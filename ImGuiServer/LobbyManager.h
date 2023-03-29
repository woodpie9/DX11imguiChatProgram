#pragma once
#include <string>
#include <vector>

#include "ServerProgram.h"
#include "../WoodnetBase/CommonDefines.h"

enum class LobbyPlayerState : int
{
	Init,
	Accept,
	InLobby,
	InChat,
	Matching,
	Matched,
	InGameSer,
	Disconnected,
};

// 클라이언트 하나를 묘사한다.
class PlayerInfo
{
public:
	LobbyPlayerState GetState() const { return state_; }
	void SetState(LobbyPlayerState val) { state_ = val; }
	NetworkObjectID GetNetId() const { return net_id_; }
	void SetNetId(NetworkObjectID val) { net_id_ = val; }
	std::string GetNickName() const { return nick_name_; }
	void SetNickName(std::string val) { nick_name_ = val; }
public:
	int life_timer_;				// int 말고 다른 좋은것이 있을꺼 같은데
	// 시간 기반으로 할까
private:
	LobbyPlayerState state_ = LobbyPlayerState::Init;
	NetworkObjectID net_id_ = 0;
	std::string nick_name_;
};


class LobbyManager
{
public:
	LobbyManager();
	~LobbyManager();

public:
	void Update();												// 업데이트 한다. 타이머를 증가시킨다.
	void HeartBeat(NetworkObjectID net_id);					// 새로운 메시지가 올 때마다 
	void NewPlayer(NetworkObjectID net_id);					// 플레이어 객체를 만든다.
	void DeletePlayer(NetworkObjectID net_id);
	bool SetPlayerNickName(NetworkObjectID net_id, char* name);

	bool SetPlayerState(NetworkObjectID net_id, LobbyPlayerState state);	// 플레이어의 상태를 변경한다.  
	LobbyPlayerState GetPlayerState(NetworkObjectID net_id);	// 플레이어의 상태를 가져온다. 
	std::string GetNickName(NetworkObjectID net_id);				// 플레이어의 닉네임을 가져온다.



private:
	void CheckHearBeat(NetworkObjectID net_id);
	// 상태 setter

private:
	std::vector<PlayerInfo> m_players_info;
};

