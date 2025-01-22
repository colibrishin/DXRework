#pragma once
#include "Source/Runtime/Core/Component/Public/Component.h"
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.h"
#include "Source/Runtime/Core/Delegation/Public/Delegation.hpp"
#include "Shape.h"

#include "egRenderComponent.generated.h"

namespace Engine::Components
{
	ECLASS(abstract, serialize)
	class ENGINE_RENDERCOMPONENT_API RenderComponent : public Engine::Abstracts::Component
	{
		GENERATE_BODY
	public:
		using Component::Component;

		void OnSerialized() override;
		void OnDeserialized() override;

#if WITH_EDITOR
		void OnUIUpdate(UIContext* const parent, const float dt);
#endif

		void SetShape(const Weak<Resources::Shape>& shape);
		[[nodiscard]] Weak<Resources::Shape> GetShape() const;
		[[nodiscard]] const MetadataPath& GetShapeMetadataPath() const;

	protected:
		RenderComponent();

	private:		
		EPROPERTY()
		MetadataPath m_shape_meta_path_;

#if WITH_EDITOR
		bool m_shape_set_dialog_ = false;
#endif

		Strong<Resources::Shape> m_shape_;
	};
}
