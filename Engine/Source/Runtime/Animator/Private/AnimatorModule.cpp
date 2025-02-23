#include "AnimatorModule.h"
#include "AnimatorModule.generated.h"

#include "Animator.h"
#include "ObjectBase.h"

MODULE_IMPL(Engine::AnimatorModule, Animator)

bool Engine::AnimatorModule::InitializeImpl()
{
	Engine::ComponentFactory::Register<Engine::Components::Animator>();
	return true;
}

bool Engine::AnimatorModule::ShutdownImpl()
{
	Engine::ComponentFactory::Unregister<Engine::Components::Animator>();
	return true;
}

bool Engine::AnimatorModule::DynamicLoadable()
{
	return true;
}
