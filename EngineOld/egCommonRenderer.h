#pragma once

namespace Engine::Manager::Graphics
{
	using CandidateTuple = std::tuple<WeakObjectBase, WeakMaterial, aligned_vector<SBs::InstanceSB>>;
	using RenderMap = concurrent_hash_map<eRenderComponentType, concurrent_vector<CandidateTuple, u_align_allocator<CandidateTuple>>>;

	void BuildRenderMap(
		const WeakScene& w_scene, RenderMap out_map[SHADER_DOMAIN_MAX], std::atomic<UINT64>& instance_count
	);
	void PremapModelImpl(
		const WeakModelRenderer& model_component, RenderMap out_map[SHADER_DOMAIN_MAX],
		std::atomic<UINT64>&     instance_count
	);
	void PremapParticleImpl(
		const WeakParticleRenderer& particle_component, RenderMap out_map[SHADER_DOMAIN_MAX],
		std::atomic<UINT64>&        instance_count
	);
}
