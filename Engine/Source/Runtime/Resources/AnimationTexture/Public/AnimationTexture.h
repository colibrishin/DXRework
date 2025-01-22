#pragma once
#include "Source/Runtime/Resources/Texture3D/Public/Texture3D.h"
#include "Source/Runtime/Managers/ResourceManager/Public/ResourceManager.hpp"

namespace Engine::Resources
{
	class AnimationTexture : public Texture3D
	{
	public:
		RESOURCE_T(RES_T_ANIMS_TEX)

		AnimationTexture(const std::vector<Strong<BoneAnimation>>& animations);

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostUpdate(const float& dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		eResourceType GetResourceType() const override;

		RESOURCE_SELF_INFER_GETTER_DECL(AnimationTexture)

		static boost::shared_ptr<AnimationTexture> Create(
			const std::string& name, const std::vector<Strong<BoneAnimation>>& anims
		)
		{
			if (const auto ncheck = Managers::ResourceManager::GetInstance().GetResource<AnimationTexture>(name).lock())
			{
				return ncheck;
			}

			const auto obj = boost::make_shared<AnimationTexture>(anims);
			Managers::ResourceManager::GetInstance().AddResource(name, obj);
			return obj;
		}

	protected:
		void loadDerived(ComPtr<ID3D12Resource>& res) override;

		bool map(char* mapped) override;

	private:
		SERIALIZE_DECL

		AnimationTexture()
			: Texture3D("", {}) {}

		constexpr static size_t s_vec4_to_mat   = 4;
		constexpr static size_t s_float_per_mat = sizeof(Matrix) / sizeof(float);

		GenericTextureDescription preEvaluateAnimations(
			const std::vector<Strong<BoneAnimation>>& anims, std::vector<std::vector<std::vector<Matrix>>>& preEvaluated
		);

		std::vector<Strong<BoneAnimation>>              m_animations_;
		std::vector<std::vector<std::vector<Matrix>>> m_evaluated_animations_;
	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::AnimationTexture)
