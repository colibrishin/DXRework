#pragma once
#include "egProjectionFrustum.hpp"
#include "egResourceManager.hpp"
#include "egSceneManager.hpp"
#include "egToolkitAPI.hpp"
#include "egCollisionManager.hpp"

namespace Engine
{
	inline Manager::ResourceManager& GetResourceManager()
	{
		return Manager::ResourceManager::GetInstance();
	}

	inline Manager::SceneManager& GetSceneManager()
	{
		return Manager::SceneManager::GetInstance();
	}

	inline Manager::ProjectionFrustum& GetProjectionFrustum()
	{
		return Manager::ProjectionFrustum::GetInstance();
	}

	inline Manager::CollisionManager& GetCollisionManager()
	{
		return Manager::CollisionManager::GetInstance();
	}

	inline Manager::Application& GetApplication()
	{
		return Manager::Application::GetInstance();
	}

	inline Manager::Graphics::D3Device& GetD3Device()
	{
		return Manager::Graphics::D3Device::GetInstance();
	}

	inline Manager::Graphics::RenderPipeline& GetRenderPipeline()
	{
		return Manager::Graphics::RenderPipeline::GetInstance();
	}

	inline Manager::Graphics::ToolkitAPI& GetToolkitAPI()
	{
		return Manager::Graphics::ToolkitAPI::GetInstance();
	}
}
