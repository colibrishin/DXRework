#pragma once
#include "Source/Runtime/Core/Component/Public/Component.h"
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.h"
#include "Source/Runtime/Core/Delegation/Public/Delegation.hpp"
#include "Source/Runtime/Resources/Material/Public/Material.h"

#include "egRenderComponent.generated.h"

DEFINE_DELEGATE(OnMaterialChange, Engine::Weak<Engine::Resources::Material>)

namespace Engine::Components
{
	class RenderComponent;
}

namespace Engine::Components
{
	ECLASS(abstract, serialize)
	class ENGINE_RENDERCOMPONENT_API RenderComponent : public Engine::Abstracts::Component
	{
		GENERATE_BODY
	public:
		DelegateOnMaterialChange onMaterialChange;

		using Component::Component;

		void SetMaterial(const Weak<Resources::Material>& material) noexcept;

		Weak<Resources::Material>    GetMaterial() const noexcept;
		const std::filesystem::path& GetMaterialMetadataPath() const noexcept;

		void OnSerialized() override;
		void OnDeserialized() override;

	protected:
		RenderComponent();

	private:		
		EPROPERTY()
		std::filesystem::path       m_mtr_meta_path_;

		Strong<Resources::Material> m_material_{};
	};
}
