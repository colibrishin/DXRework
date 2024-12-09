#pragma once

namespace Engine::Manager::Graphics
{
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
