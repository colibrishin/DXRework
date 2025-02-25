#include "PhysicsManagerModule.h"
#include "PhysicsManagerModule.generated.h"

#include "CollisionDetector.h"
#include "ConstraintSolver.h"
#include "EngineEntryPoint.h"
#include "Graviton.h"
#include "PhysicsManager.h"

MODULE_IMPL(Engine::PhysicsManagerModule, PhysicsManager);

bool Engine::PhysicsManagerModule::InitializeImpl()
{
	CoreLoop::AddManager(
		CoreLoop::LOOP_TYPE_PHYSICS,
		&Managers::Graviton::GetInstance,
		&Managers::CollisionDetector::GetInstance,
		&Managers::ConstraintSolver::GetInstance,
		&Managers::PhysicsManager::GetInstance);

	return true;
}

bool Engine::PhysicsManagerModule::ShutdownImpl()
{
	CoreLoop::RemoveManager(
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
