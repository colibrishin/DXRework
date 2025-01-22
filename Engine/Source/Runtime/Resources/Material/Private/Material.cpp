#include "../Public/Material.h"
#include "Material.generated.h"

#include <DirectXColors.h>

#include "ModuleManager/Public/ModuleManager.h"

#include "SceneManager/Public/SceneManager.h"

#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.h"
#include "Source/Runtime/Resources/AtlasAnimation/Public/AtlasAnimation.h"
#include "Source/Runtime/Resources/BoneAnimation/Public/BoneAnimation.h"
#include "Source/Runtime/Resources/Shader/Public/Shader.h"
#include "Source/Runtime/Resources/Shape/Public/Shape.h"
#include "Source/Runtime/Resources/Texture/Public/Texture.h"
#include "UIHelpersResourceManager.h"

namespace Engine::Resources
{
	Material::Material(const SBs::MaterialSB& material) :
	Resource(""),
	m_material_sb_(material) {}

#if WITH_EDITOR
	void Material::OnUIUpdate(UIContext* const parent, const float dt)
	{
		if (parent)
		{
			Resource::OnUIUpdate(parent, dt);

			UIInterface& ui = UIInterfaceAccessor::GetInterface();

			*parent |= ui.NewLabelAndFloat({"Specular Power", m_material_sb_.specularPower, 0.1f, 0.f, std::numeric_limits<float>::max(), true});
			*parent |= ui.NewLabelAndFloat({"Reflection Scale", m_material_sb_.reflectionScale, 0.1f, 0.f, std::numeric_limits<float>::max(), true});
			*parent |= ui.NewLabelAndFloat({"Refraction Scale", m_material_sb_.refractionScale, 0.1f, 0.f, std::numeric_limits<float>::max(), true});
			*parent |= ui.NewLabelAndFloat({"Reflection Translation", m_material_sb_.reflectionTranslation, 0.1f, 0.f, 0.f, true});
			*parent |= ui.NewLabelAndVec4({"Override Color", &m_material_sb_.overrideColor.x, 0.01f, 0.1, 1.f, true});
			*parent |= ui.NewLabelAndVec4({"Specular Color", &m_material_sb_.specularColor.x, 0.01f, 0.1, 1.f, true});
			*parent |= ui.NewLabelAndVec3({"Clip Plane", &m_material_sb_.clipPlane.x, 0.01f, 0.f, 0.f, true});
			*parent |= ui.NewCheckbox({"Repeat Texture", reinterpret_cast<bool&>(m_material_sb_.repeatTexture.value)});
		}
	}
#endif

	void Material::PreUpdate(const float dt) {}

	void Material::Update(const float dt) {}

	void Material::PostUpdate(const float dt) {}

	void Material::FixedUpdate(const float dt) {}

	void Material::OnSerialized()
	{
		Resource::OnSerialized();
		Load();
	}

	void Material::OnDeserialized()
	{
		Resource::OnDeserialized();
	}

	const SBs::MaterialSB& Material::GetMaterialSB() const
	{
		return m_material_sb_;
	}

	Material::Material()
		: Resource("") {}

	void Material::Load_INTERNAL() {}

	void Material::Unload_INTERNAL() {}
}
