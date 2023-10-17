#pragma once
#include "egResourceManager.hpp"
#include "egSceneManager.hpp"

namespace Engine
{
	inline Manager::ResourceManager* GetResourceManager()
	{
		return Manager::ResourceManager::GetInstance();
	}

	inline Manager::SceneManager* GetSceneManager()
	{
		return Manager::SceneManager::GetInstance();
	}
}
