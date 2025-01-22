#pragma once
#include "Source/Runtime/Core/StructuredBuffer/Public/StructuredBuffer.h"
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "Source/Runtime/Core/Resource/Public/Resource.h"
#include "Source/Runtime/Resources/Shader/Public/Shader.h"

#include <string>
#include <map>

#include "MaterialSB.h"

#include "Material.generated.h"

namespace Engine::Resources
{
	ECLASS(resource, serialize)
	class ENGINE_MATERIAL_API Material final : public Abstracts::Resource
	{
		GENERATE_BODY
	public:
		Material(const Graphics::SBs::MaterialSB& material);

#if WITH_EDITOR
		void OnUIUpdate(UIContext* const parent, const float dt) override;
#endif
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void PostUpdate(const float dt) override;
		void FixedUpdate(const float dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		[[nodiscard]] const Graphics::SBs::MaterialSB& GetMaterialSB() const;

	private:
		Material();

		EPROPERTY()
		Graphics::SBs::MaterialSB m_material_sb_;
	};
}