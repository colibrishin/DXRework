#pragma once
#include <filesystem>
#include "Source/Runtime/Resources/BaseAnimation/Public/BaseAnimation.h"
#include "AtlasAnimationPrimitive.h"

#include "AtlasAnimation.generated.h"

namespace Engine::Resources
{
	using namespace Graphics;

	ECLASS(resource)
	class ENGINE_ATLASANIMATION_API AtlasAnimation : public BaseAnimation
	{
		GENERATE_BODY
	public:
		AtlasAnimation(const std::filesystem::path& xml_path);

		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostUpdate(const float dt) override;

		void OnDeserialized() override;
		void OnSerialized() override;

		void __vectorcall GetFrame(float dt, AtlasFramePrimitive& out) const;

		[[nodiscard]] static AtlasAnimationPrimitive ParseXML(const std::filesystem::path& path);

	protected:
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

		AtlasAnimation() = default;

		EPROPERTY()
		std::filesystem::path m_xml_path_;
		
		EPROPERTY()
		AtlasAnimationPrimitive m_primitive_;
	};
}
