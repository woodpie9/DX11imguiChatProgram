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
	LobbyPlayerState get_state() const { return state; }
	void set_state(LobbyPlayerState val) { state = val; }
	NetworkObjectID get_net_id() const { return net_id; }
	void set_net_id(NetworkObjectID val) { net_id = val; }
	std::string get_nick_name() const { return nick_name; }
	void set_nick_name(std::string val) { nick_name = val; }
public:
	int life_timer;				// int 말고 다른 좋은것이 있을꺼 같은데
	// 시간 기반으로 할까
private:
	LobbyPlayerState state = LobbyPlayerState::Init;
	NetworkObjectID net_id = 0;
	std::string nick_name;
};


class LobbyManager
{
public:
	LobbyManager();
	~LobbyManager();

public:
	void update();											// 업데이트 한다. 타이머를 증가시킨다.
	void new_player(NetworkObjectID NetID);					// 플레이어 객체를 만든다.
	void delete_player(NetworkObjectID NetID);
	bool set_player_state(NetworkObjectID NetID, LobbyPlayerState State);	// 플레이어의 상태를 변경한다.  
	bool set_player_nick_name(NetworkObjectID NetID, char* name);
	void heart_beat(NetworkObjectID NetID);					// 새로운 메시지가 올 때마다 

	LobbyPlayerState get_player_state(NetworkObjectID NetID);	// 플레이어의 상태를 가져온다. 
	std::string get_nick_name(NetworkObjectID NetID);				// 플레이어의 닉네임을 가져온다.



private:
	void check_hear_beat(NetworkObjectID NetID);

private:
	std::vector<PlayerInfo> m_players_info;
};

