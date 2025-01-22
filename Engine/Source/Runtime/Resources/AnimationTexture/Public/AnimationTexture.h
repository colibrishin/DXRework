#pragma once
#include "Source/Runtime/Resources/Texture3D/Public/Texture3D.h"

#include "AnimationTexture.generated.h"

namespace Engine::Resources
{
	class BoneAnimation;
}

namespace Engine::Resources
{
	ECLASS(resource, serialize)
	class ENGINE_ANIMATIONTEXTURE_API AnimationTexture : public Texture3D
	{
		GENERATE_BODY
	public:
		AnimationTexture(const std::vector<Strong<BoneAnimation>>& animations);

		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostUpdate(const float dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		const std::vector<Weak<BoneAnimation>>& GetAnimations() const;

	protected:
		void Load_INTERNAL() override;

	private:
		AnimationTexture()
			: Texture3D("", {}) {}

		constexpr static size_t s_vec4_to_mat   = 4;
		constexpr static size_t s_float_per_mat = sizeof(Matrix) / sizeof(float);

		void Map() override;
		GenericTextureDescription preEvaluateAnimations(
			const std::vector<Strong<BoneAnimation>>& anims, std::vector<std::vector<std::vector<Matrix>>>& preEvaluated
		);

		EPROPERTY()
		std::vector<Strong<BoneAnimation>>            m_animations_;
		std::vector<Weak<BoneAnimation>>              m_cached_animations_;
		
		std::vector<std::vector<std::vector<Matrix>>> m_evaluated_animations_;
	};
}
