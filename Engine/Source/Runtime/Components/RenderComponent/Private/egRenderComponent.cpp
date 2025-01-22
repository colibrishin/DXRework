#include "../Public/egRenderComponent.h"
#include "egRenderComponent.generated.h"

#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.h"

namespace Engine::Components
{
	void RenderComponent::OnSerialized()
	{
		Component::OnSerialized();

		if (m_shape_) 
		{
			m_shape_meta_path_ = m_shape_->GetMetadataPath();
		}
	}

	void RenderComponent::OnDeserialized()
	{
		Component::OnDeserialized();

		if (const Strong<Resources::Shape>& shape = Resources::Shape::GetByMetadataPath(m_shape_meta_path_).lock())
		{
			m_shape_ = shape;
		}
	}

	void RenderComponent::SetShape(const Weak<Resources::Shape>& shape)
	{
		if (const Strong<Resources::Shape>& locked = shape.lock())
		{
			m_shape_ = locked;
			m_shape_meta_path_ = locked->GetMetadataPath();
		}
	}

	Weak<Resources::Shape> RenderComponent::GetShape() const
	{
		return m_shape_;
	}

	const MetadataPath& RenderComponent::GetShapeMetadataPath() const
	{
		return m_shape_meta_path_;
	}

	RenderComponent::RenderComponent()
		: Component({}) {}
}
