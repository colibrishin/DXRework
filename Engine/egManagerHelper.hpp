#pragma once
#include "egApplication.hpp"
#include "egProjectionFrustum.hpp"
#include "egResourceManager.hpp"
#include "egSceneManager.hpp"
#include "egToolkitAPI.hpp"
#include "egCollisionDetector.hpp"
#include "egConstraintSolver.hpp"
#include "egDebugger.hpp"
#include "egLerpManager.hpp"
#include "egMouseManager.hpp"
#include "egPhysicsManager.hpp"
#include "egShadowManager.hpp"
#include "egTaskScheduler.hpp"

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

	inline Manager::CollisionDetector& GetCollisionDetector()
	{
		return Manager::CollisionDetector::GetInstance();
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

	inline Manager::Physics::LerpManager& GetLerpManager()
	{
		return Manager::Physics::LerpManager::GetInstance();
	}

	inline Manager::Physics::PhysicsManager& GetPhysicsManager()
	{
		return Manager::Physics::PhysicsManager::GetInstance();
	}

	inline Manager::Physics::ConstraintSolver& GetConstraintSolver()
	{
		return Manager::Physics::ConstraintSolver::GetInstance();
	}

	inline Manager::Debugger& GetDebugger()
	{
		return Manager::Debugger::GetInstance();
	}

	inline Manager::TaskScheduler& GetTaskScheduler()
	{
		return Manager::TaskScheduler::GetInstance();
	}

	inline Manager::MouseManager& GetMouseManager()
	{
		return Manager::MouseManager::GetInstance();
	}

	inline Manager::Graphics::ShadowManager& GetShadowManager()
	{
		return Manager::Graphics::ShadowManager::GetInstance();
	}
}
