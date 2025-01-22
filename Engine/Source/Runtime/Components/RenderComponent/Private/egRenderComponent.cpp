#include "../Public/egRenderComponent.h"
#include "egRenderComponent.generated.h"

#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.h"
#include "UIHelpersResourceManager.h"

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

	void RenderComponent::OnUIUpdate(UIContext* const parent, const float dt)
	{
		if (parent)
		{
			Component::OnUIUpdate(parent, dt);

			static std::string empty_string;
			UIInterface& ui = UIInterfaceAccessor::GetInterface();
			*parent |= ui.NewLabelAndText({ "Shape", m_shape_ ? const_cast<std::string&>(m_shape_->GetName()) : empty_string, false });
			(*parent |= ui.NewButton("Set Shape")).SetFunction([&]()
				{
					m_shape_set_dialog_ = !m_shape_set_dialog_;
				});

			if (m_shape_set_dialog_)
			{
				if (Weak<Engine::Abstracts::Resource> resource_to_load;
					UIHelpers::SingleResourceSelectionDialogInclusion<RenderComponent, Resources::Shape>(GetSharedPtr<Entity>(), resource_to_load))
				{
					if (const Strong<Engine::Abstracts::Resource>& shape = resource_to_load.lock())
					{
						SetShape(shape->GetSharedPtr<Resources::Shape>());
					}

					m_shape_set_dialog_ = false;
				}
			}
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
