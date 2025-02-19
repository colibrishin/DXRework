#pragma once
#include "Component/Public/Component.h"
#include "ObjectBase/Public/ObjectBase.h"

#include "RenderComponent.generated.h"

namespace Engine::Components
{
	ECLASS(component, abstract, serialize)
	class ENGINE_RENDERCOMPONENT_API RenderComponent : public Engine::Abstracts::Component
	{
		GENERATE_BODY
	public:
		using Component::Component;

		void OnSerialized() override;
		void OnDeserialized() override;

#if WITH_EDITOR
		void OnUIUpdate(UIContext* const parent, const float dt) override;
#endif

	protected:
		RenderComponent();
	};
}
