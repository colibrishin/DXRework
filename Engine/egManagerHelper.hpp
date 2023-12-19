#pragma once
#include "egApplication.h"
#include "egCollisionDetector.h"
#include "egConstraintSolver.h"
#include "egDebugger.hpp"
#include "egLerpManager.h"
#include "egMouseManager.h"
#include "egPhysicsManager.h"
#include "egProjectionFrustum.h"
#include "egResourceManager.hpp"
#include "egRenderPipeline.h"
#include "egSceneManager.hpp"
#include "egShadowManager.hpp"
#include "egTaskScheduler.h"
#include "egToolkitAPI.h"

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
} // namespace Engine
