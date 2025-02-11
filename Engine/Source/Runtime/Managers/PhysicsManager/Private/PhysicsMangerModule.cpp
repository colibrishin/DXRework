#include "PhysicsManagerModule.h"
#include "PhysicsManagerModule.generated.h"

#include "CollisionDetector.h"
#include "ConstraintSolver.h"
#include "Graviton.h"
#include "PhysicsManager.h"

#include "CoreModuel/Public/CoreModule.h"
#include "ModuleManager/Public/ModuleManager.h"

MODULE_IMPL(Engine::PhysicsManagerModule, PhysicsManager);

bool Engine::PhysicsManagerModule::InitializeImpl()
{
	CoreModule::GetContext().AddManager(
		CoreLoop::LOOP_TYPE_PHYSICS,
		&Managers::Graviton::GetInstance,
		&Managers::CollisionDetector::GetInstance,
		&Managers::ConstraintSolver::GetInstance,
		&Managers::PhysicsManager::GetInstance);

	return true;
}

bool Engine::PhysicsManagerModule::ShutdownImpl()
{
	CoreModule::GetContext().RemoveManager(
		CoreLoop::LOOP_TYPE_PHYSICS,
		&Managers::Graviton::GetInstance,
		&Managers::CollisionDetector::GetInstance,
		&Managers::ConstraintSolver::GetInstance,
		&Managers::PhysicsManager::GetInstance);

	return true;
}

bool Engine::PhysicsManagerModule::DynamicLoadable()
{
	return true;	
}
