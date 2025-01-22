#include "../Public/egRenderComponent.h"

#include "Source/Runtime/Managers/ResourceManager/Public/ResourceManager.hpp"

namespace Engine::Components
{
	void RenderComponent::SetMaterial(const Weak<Resources::Material>& material) noexcept
	{
		if (const auto mtr = material.lock())
		{
			m_material_      = mtr;
			m_mtr_meta_path_ = mtr->GetMetadataPath();
			onMaterialChange.Broadcast(mtr);
		}
	}

	eRenderComponentType RenderComponent::GetRenderType() const noexcept
	{
		return m_type_;
	}

	Weak<Resources::Material> RenderComponent::GetMaterial() const noexcept
	{
		return m_material_;
	}

	const std::filesystem::path& RenderComponent::GetMaterialMetadataPath() const noexcept
	{
		return m_mtr_meta_path_;
	}

	eRenderComponentType RenderComponent::GetType() const noexcept
	{
		return m_type_;
	}

	void RenderComponent::OnSerialized()
	{
		Component::OnSerialized();
	}

	void RenderComponent::OnDeserialized()
	{
		Component::OnDeserialized();

		if (const auto res_check = Resources::Material::GetByMetadataPath(m_mtr_meta_path_).lock();
			res_check && !res_check->GetMetadataPath().empty())
		{
			m_material_ = res_check;
		}
	}

	RenderComponent::RenderComponent()
		: Component(COM_T_RENDERER, {}),
		  m_type_(RENDER_COM_T_UNK) {}
}
