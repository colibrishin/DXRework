#pragma once
#include <algorithm>
#include <array>
#include <random>
#include "egComputeShader.h"

namespace Client::ComputeShaders
{
	class ParticleCompute final : public Resources::ComputeShader
	{
	public:
		explicit ParticleCompute()
			: ComputeShader("cs_particle", "cs_particle.hlsl", {32, 32, 1}) {}

	protected:
		SERIALIZE_DECL

		void preDispatch(
			ID3D12GraphicsCommandList1* list, const DescriptorPtr& heap, Graphics::SBs::LocalParamSB& param
		) override;
		void postDispatch(
			ID3D12GraphicsCommandList1* list, const DescriptorPtr& heap, Graphics::SBs::LocalParamSB& param
		) override;
		void loadDerived() override;
		void unloadDerived() override;

		void OnImGui(const StrongParticleRenderer& pr) override;

		void SetScaling(bool scaling, Graphics::ParamBase& config);
		void SetScalingParam(float min, float max, Graphics::ParamBase& config);

		void LinearSpread(
			const Vector3&             local_min, const Vector3& local_max, InstanceParticles& particles,
			const Graphics::ParamBase& config
		);

	private:
		// int
		constexpr static size_t scaling_active_slot = 1;
		constexpr static size_t random_value_slot   = 2;

		// float
		constexpr static size_t dt_slot          = 2;
		constexpr static size_t scaling_min_slot = 3;
		constexpr static size_t scaling_max_slot = 4;

		std::mt19937_64 getRandomEngine() const;

		constexpr static size_t random_texture_count = 3;
		constexpr static size_t random_texture_size  = 500 * 500;

		std::array<std::shared_ptr<Resources::Texture2D>, random_texture_count> m_noises_;
	};
}

BOOST_CLASS_EXPORT_KEY(Client::ComputeShaders::ParticleCompute)
