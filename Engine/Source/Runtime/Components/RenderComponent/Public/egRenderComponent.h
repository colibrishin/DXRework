#pragma once
#include "Source/Runtime/Delegation/Public/Delegation.hpp"
#include "Source/Runtime/Abstracts/CoreComponent/Public/Component.h"
#include <Source/Runtime/TypeLibrary/Public/TypeLibrary.h>

DEFINE_DELEGATE(OnMaterialChange, Engine::Weak<Engine::Resources::Material>)

// Static Render Component type, this should be added to every render component
#define RENDER_COM_T(enum_val) static constexpr eRenderComponentType rctype = enum_val;

namespace Engine 
{
	enum eRenderComponentType : uint8_t 
	{
		RENDER_COM_T_UNK = 0,
		RENDER_COM_T_MODEL,
		RENDER_COM_T_PARTICLE
	};
}

namespace Engine::Components
{
	class RenderComponent : public Engine::Abstracts::Component
	{
	public:
		COMPONENT_T(COM_T_RENDERER)

		DelegateOnMaterialChange onMaterialChange;

		explicit RenderComponent(eRenderComponentType type, const Weak<Engine::Abstracts::ObjectBase>& owner)
			: Component(COM_T_RENDERER, owner),
			  m_type_(type),
			  m_mtr_meta_path_() {}

		void SetMaterial(const Weak<Resources::Material>& material) noexcept;

		eRenderComponentType         GetRenderType() const noexcept;
		Weak<Resources::Material>                 GetMaterial() const noexcept;
		const std::filesystem::path& GetMaterialMetadataPath() const noexcept;
		eRenderComponentType         GetType() const noexcept;

		void OnSerialized() override;
		void OnDeserialized() override;

	private:
		SERIALIZE_DECL
		RenderComponent();

		Strong<Resources::Material>       m_material_;
		eRenderComponentType m_type_;
		std::filesystem::path m_mtr_meta_path_;
	};
}

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Engine::Components::RenderComponent)
