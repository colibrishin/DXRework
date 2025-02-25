#include "RenderComponent.h"
#include "RenderComponent.generated.h"

#include "ResourceManager.h"
#include "UIHelpersResourceManager.h"

namespace Engine::Components
{
	void RenderComponent::OnSerialized()
	{
		Component::OnSerialized();
	}

	void RenderComponent::OnDeserialized()
	{
		Component::OnDeserialized();
	}

#if WITH_EDITOR
	void RenderComponent::OnUIUpdate(UIContext* const parent, const float dt)
	{
		Component::OnUIUpdate(parent, dt);
	}
#endif

	RenderComponent::RenderComponent()
		: Component({}) {}
}
