#include "AnimatorModule.h"
#include "AnimatorModule.generated.h"

#include "../Public/Animator.h"

#include "ModuleManager/Public/ModuleManager.h"
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.h"

MODULE_IMPL(Engine::AnimatorModule, Animator)

bool Engine::AnimatorModule::InitializeImpl()
{
	Abstracts::ObjectBase::RegisterComponentFactory(Components::Animator::StaticTypeName(), [](const Weak<Abstracts::ObjectBase>& owner)
	{
		if (const Strong<Abstracts::ObjectBase>& locked = owner.lock()) 
		{
			locked->AddComponent<Components::Animator>();
		}
	});

	return true;
}

bool Engine::AnimatorModule::ShutdownImpl()
{
	Abstracts::ObjectBase::UnregisterComponentFactory(Components::Animator::StaticTypeName());

	return true;
}

bool Engine::AnimatorModule::DynamicLoadable()
{
	return true;
}
