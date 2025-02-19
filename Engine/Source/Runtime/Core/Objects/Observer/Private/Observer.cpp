#include "Objects/Observer/Public/Observer.h"
#include "Observer.generated.h"
#include "ObjectBase/Public/ObjectBase.h"

#include "Components/Transform/Public/Transform.h"

namespace Engine::Objects
{
	Observer::Observer()
		: ObjectBase(DEF_OBJ_T_OBSERVER) {}

	void Observer::Initialize()
	{
		ObjectBase::Initialize();

		const auto tr = AddComponent<Components::Transform>().lock();
		//AddComponent<Components::ObserverController>();
	}

	Observer::~Observer() {}

	void Observer::PreUpdate(const float dt)
	{
		ObjectBase::PreUpdate(dt);
	}

	void Observer::Update(const float dt)
	{
		ObjectBase::Update(dt);
	}

	void Observer::PreRender(const float dt)
	{
		ObjectBase::PreRender(dt);
	}

	void Observer::Render(const float dt)
	{
		ObjectBase::Render(dt);
	}

	void Observer::PostRender(const float dt)
	{
		ObjectBase::PostRender(dt);
	}

	void Observer::FixedUpdate(const float dt)
	{
		ObjectBase::FixedUpdate(dt);
	}
} // namespace Engine::Objects