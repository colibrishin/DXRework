#include "PhysicsManagerModule.h"
#include "PhysicsManagerModule.generated.h"

#include "CollisionDetector.h"
#include "ConstraintSolver.h"
#include "Graviton.h"
#include "PhysicsManager.h"

#include "CoreModuel/Public/CoreModule.h"
#include "ModuleManager/Public/ModuleManager.h"

MODULE_IMPL(Engine::PhysicsManagerModule, PhysicsManager);

void Engine::PhysicsManagerModule::Initialize()
{
	CoreModule::GetContext().AddManager(
		CoreLoop::LOOP_TYPE_PHYSICS,
		&Managers::Graviton::GetInstance,
		&Managers::CollisionDetector::GetInstance,
		&Managers::ConstraintSolver::GetInstance,
		&Managers::PhysicsManager::GetInstance);
}

void Engine::PhysicsManagerModule::Shutdown()
{
	CoreModule::GetContext().RemoveManager(
		CoreLoop::LOOP_TYPE_PHYSICS,
		&Managers::Graviton::GetInstance,
		&Managers::CollisionDetector::GetInstance,
		&Managers::ConstraintSolver::GetInstance,
		&Managers::PhysicsManager::GetInstance);
}

bool Engine::PhysicsManagerModule::DynamicLoadable()
{
	return true;	
}
