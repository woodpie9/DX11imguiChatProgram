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

// Ŭ���̾�Ʈ �ϳ��� �����Ѵ�.
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
	int life_timer_;				// int ���� �ٸ� �������� ������ ������
	// �ð� ������� �ұ�
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
	void Update();												// ������Ʈ �Ѵ�. Ÿ�̸Ӹ� ������Ų��.
	void HeartBeat(NetworkObjectID net_id);					// ���ο� �޽����� �� ������ 
	void NewPlayer(NetworkObjectID net_id);					// �÷��̾� ��ü�� �����.
	void DeletePlayer(NetworkObjectID net_id);
	bool SetPlayerNickName(NetworkObjectID net_id, char* name);

	bool SetPlayerState(NetworkObjectID net_id, LobbyPlayerState state);	// �÷��̾��� ���¸� �����Ѵ�.  
	LobbyPlayerState GetPlayerState(NetworkObjectID net_id);	// �÷��̾��� ���¸� �����´�. 
	std::string GetNickName(NetworkObjectID net_id);				// �÷��̾��� �г����� �����´�.



private:
	void CheckHearBeat(NetworkObjectID net_id);
	// ���� setter

private:
	std::vector<PlayerInfo> m_players_info;
};

