#include "../Public/Animator.h"
#include "AnimatorModule.h"

#include "ModuleManager/Public/ModuleManager.h"
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.h"

MODULE_IMPL(Engine::AnimatorModule, Animator)

void Engine::AnimatorModule::Initialize()
{
	Abstracts::ObjectBase::RegisterComponentFactory(Components::Animator::StaticTypeName(), [](const Weak<Abstracts::ObjectBase>& owner)
	{
		if (const Strong<Abstracts::ObjectBase>& locked = owner.lock()) 
		{
			locked->AddComponent<Components::Animator>();
		}
	});
}

void Engine::AnimatorModule::Shutdown()
{
	Abstracts::ObjectBase::UnregisterComponentFactory(Components::Animator::StaticTypeName());
}

bool Engine::AnimatorModule::DynamicLoadable()
{
	return true;
}
