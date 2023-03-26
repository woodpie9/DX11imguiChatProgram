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
	LobbyPlayerState get_state() const { return state; }
	void set_state(LobbyPlayerState val) { state = val; }
	NetworkObjectID get_net_id() const { return net_id; }
	void set_net_id(NetworkObjectID val) { net_id = val; }
	std::string get_nick_name() const { return nick_name; }
	void set_nick_name(std::string val) { nick_name = val; }
public:
	int life_timer;				// int ���� �ٸ� �������� ������ ������
	// �ð� ������� �ұ�
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
	void update();											// ������Ʈ �Ѵ�. Ÿ�̸Ӹ� ������Ų��.
	void new_player(NetworkObjectID NetID);					// �÷��̾� ��ü�� �����.
	void delete_player(NetworkObjectID NetID);
	bool set_player_state(NetworkObjectID NetID, LobbyPlayerState State);	// �÷��̾��� ���¸� �����Ѵ�.  
	bool set_player_nick_name(NetworkObjectID NetID, char* name);
	void heart_beat(NetworkObjectID NetID);					// ���ο� �޽����� �� ������ 

	LobbyPlayerState get_player_state(NetworkObjectID NetID);	// �÷��̾��� ���¸� �����´�. 
	std::string get_nick_name(NetworkObjectID NetID);				// �÷��̾��� �г����� �����´�.



private:
	void check_hear_beat(NetworkObjectID NetID);

private:
	std::vector<PlayerInfo> m_players_info;
};

