#pragma once
#include "Component/Public/Component.h"
#include "ObjectBase/Public/ObjectBase.h"
#include "Shape.h"
#include "RenderComponent.h"

#include "ShapeRenderComponent.generated.h"

namespace Engine::Components
{
	ECLASS(component, abstract, serialize)
	class ENGINE_SHAPERENDERCOMPONENT_API ShapeRenderComponent : public Engine::Components::RenderComponent
	{
		GENERATE_BODY
	public:
		using RenderComponent::RenderComponent;

		void OnSerialized() override;
		void OnDeserialized() override;

#if WITH_EDITOR
		void OnUIUpdate(UIContext* const parent, const float dt);
#endif

		void SetShape(const Weak<Resources::Shape>& shape);
		[[nodiscard]] Weak<Resources::Shape> GetShape() const;
		[[nodiscard]] const MetadataPath& GetShapeMetadataPath() const;

	protected:
		ShapeRenderComponent();

	private:		
		EPROPERTY()
		MetadataPath m_shape_meta_path_;

#if WITH_EDITOR
		bool m_shape_set_dialog_ = false;
#endif

		Strong<Resources::Shape> m_shape_;
	};
}
