#include "LobbyManager.h"

LobbyManager::LobbyManager()
{
	m_players_info.clear();
	m_players_info.reserve(8);
}

LobbyManager::~LobbyManager()
{
	m_players_info.clear();
}

void LobbyManager::update()
{
	for (auto player : m_players_info)
	{
		player.life_timer++;
	}
}

void LobbyManager::new_player(NetworkObjectID NetID)
{
	// 새로운 플레이어의 정보를 담을 구조체를 만들어서 벡터에 넣는다.
	PlayerInfo newPlayer;

	newPlayer.set_state(LobbyPlayerState::Accept);
	newPlayer.set_net_id(NetID);
	newPlayer.set_nick_name("");
	newPlayer.life_timer = 0;

	m_players_info.push_back(newPlayer);
}

void LobbyManager::delete_player(NetworkObjectID NetID)
{
	for (int i = 0; i < m_players_info.size(); i++)
	{
		if (m_players_info[i].get_net_id() == NetID)
		{
			m_players_info.erase(m_players_info.begin() + i);
			return;
		}
	}
}

bool LobbyManager::set_player_state(NetworkObjectID NetID, LobbyPlayerState State)
{
	for (auto player : m_players_info)
	{
		if (player.get_net_id() == NetID)
		{
			player.set_state(State);
			return true;
		}
	}
}

bool LobbyManager::set_player_nick_name(NetworkObjectID NetID, char* name)
{
	for (int i = 0; i < m_players_info.size(); i++)
	{
		if (m_players_info[i].get_net_id() == NetID)
		{
			m_players_info[i].set_nick_name(name);

			return true;
		}
	}

	return false;
}

void LobbyManager::heart_beat(NetworkObjectID NetID)
{
	for (int i = 0; i < m_players_info.size(); i++)
	{
		if (m_players_info[i].get_net_id() == NetID)
		{
			m_players_info[i].life_timer = 0;
			return;
		}
	}
}

LobbyPlayerState LobbyManager::get_player_state(NetworkObjectID NetID)
{
	for (const auto player : m_players_info)
	{
		if (player.get_net_id() == NetID)
		{
			return player.get_state();
		}
	}
}

std::string LobbyManager::get_nick_name(NetworkObjectID NetID)
{
	for (int i = 0; i < m_players_info.size(); i++)
	{
		if (m_players_info[i].get_net_id() == NetID)
		{
			return m_players_info[i].get_nick_name();
		}
	}
}

void LobbyManager::check_hear_beat(NetworkObjectID NetID)
{
	for (const auto player : m_players_info)
	{
		if (player.life_timer > 10000000)
		{
			// TODO : 일정 이상 응답이 새로 없으면 살아있는지 물어보자.
		}
	}
}
