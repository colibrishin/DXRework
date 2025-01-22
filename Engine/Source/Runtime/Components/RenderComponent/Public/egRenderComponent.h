#pragma once
#include "Source/Runtime/Core/Component/Public/Component.h"
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.hpp"
#include "Source/Runtime/Core/Delegation/Public/Delegation.hpp"
#include "Source/Runtime/Resources/Material/Public/Material.h"

DEFINE_DELEGATE(OnMaterialChange, Engine::Weak<Engine::Resources::Material>)

namespace Engine::Components
{
	class RenderComponent;
}

POLYMORPHIC_TYPE_MAP(ENGINE_RENDERCOMPONENT_API, Engine::Components::RenderComponent, Engine::Abstracts::Component)

namespace Engine::Components
{
	class ENGINE_RENDERCOMPONENT_API RenderComponent : public Engine::Abstracts::Component
	{
	public:
		INLINE_COMPILE_TIME_TYPENAME(RenderComponent)

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
		Strong<Resources::Material> m_material_{};
		std::filesystem::path       m_mtr_meta_path_;
	};
}
