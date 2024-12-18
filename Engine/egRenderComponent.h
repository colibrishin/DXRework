#pragma once
#include "egComponent.h"
#include "egDelegate.hpp"

DEFINE_DELEGATE(OnMaterialChange, Engine::Weak<Engine::Resources::Material>)

namespace Engine::Components::Base
{
	class RenderComponent : public Abstract::Component
	{
	public:
		COMPONENT_T(COM_T_RENDERER)

		DelegateOnMaterialChange onMaterialChange;

		explicit RenderComponent(eRenderComponentType type, const WeakObjectBase& owner)
			: Component(COM_T_RENDERER, owner),
			  m_type_(type),
			  m_mtr_meta_path_() {}

		void SetMaterial(const WeakMaterial& material) noexcept;

		eRenderComponentType         GetRenderType() const noexcept;
		WeakMaterial                 GetMaterial() const noexcept;
		const std::filesystem::path& GetMaterialMetadataPath() const noexcept;
		eRenderComponentType         GetType() const noexcept;

		void OnSerialized() override;
		void OnDeserialized() override;
		void OnImGui() override;

	private:
		SERIALIZE_DECL
		RenderComponent();

		StrongMaterial       m_material_;
		eRenderComponentType m_type_;
		std::string          m_mtr_meta_path_str_;

		// non-serialized
		std::filesystem::path m_mtr_meta_path_;
	};
}

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Engine::Components::Base::RenderComponent)
