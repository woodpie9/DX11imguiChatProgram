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

void LobbyManager::Update()
{
	for (auto player : m_players_info)
	{
		player.life_timer_++;
	}
}

void LobbyManager::NewPlayer(NetworkObjectID net_id)
{
	// ���ο� �÷��̾��� ������ ���� ����ü�� ���� ���Ϳ� �ִ´�.
	PlayerInfo newPlayer;

	newPlayer.SetState(LobbyPlayerState::Accept);
	newPlayer.SetNetId(net_id);
	newPlayer.SetNickName("");
	newPlayer.life_timer_ = 0;

	m_players_info.push_back(newPlayer);
}

void LobbyManager::DeletePlayer(NetworkObjectID net_id)
{
	for (int i = 0; i < m_players_info.size(); i++)
	{
		if (m_players_info[i].GetNetId() == net_id)
		{
			m_players_info.erase(m_players_info.begin() + i);
			return;
		}
	}
}

bool LobbyManager::SetPlayerState(NetworkObjectID net_id, LobbyPlayerState state)
{
	for (auto player : m_players_info)
	{
		if (player.GetNetId() == net_id)
		{
			player.SetState(state);
			return true;
		}
	}
}

bool LobbyManager::SetPlayerNickName(NetworkObjectID net_id, char* name)
{
	for (int i = 0; i < m_players_info.size(); i++)
	{
		if (m_players_info[i].GetNetId() == net_id)
		{
			m_players_info[i].SetNickName(name);

			return true;
		}
	}

	return false;
}

void LobbyManager::HeartBeat(NetworkObjectID net_id)
{
	for (int i = 0; i < m_players_info.size(); i++)
	{
		if (m_players_info[i].GetNetId() == net_id)
		{
			m_players_info[i].life_timer_ = 0;
			return;
		}
	}
}

LobbyPlayerState LobbyManager::GetPlayerState(NetworkObjectID net_id)
{
	for (const auto player : m_players_info)
	{
		if (player.GetNetId() == net_id)
		{
			return player.GetState();
		}
	}
}

std::string LobbyManager::GetNickName(NetworkObjectID net_id)
{
	for (int i = 0; i < m_players_info.size(); i++)
	{
		if (m_players_info[i].GetNetId() == net_id)
		{
			return m_players_info[i].GetNickName();
		}
	}
}

void LobbyManager::CheckHearBeat(NetworkObjectID net_id)
{
	for (const auto player : m_players_info)
	{
		if (player.life_timer_ > 10000000)
		{
			// TODO : ���� �̻� ������ ���� ������ ����ִ��� �����.
		}
	}
}
