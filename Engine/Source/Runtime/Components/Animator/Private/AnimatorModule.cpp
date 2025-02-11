#include "AnimatorModule.h"
#include "AnimatorModule.generated.h"

#include "../Public/Animator.h"

#include "ModuleManager/Public/ModuleManager.h"
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.h"

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
