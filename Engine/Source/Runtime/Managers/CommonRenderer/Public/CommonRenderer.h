#include <tuple>

#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/Core/Components/RenderComponent/Public/egRenderComponent.h"
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.hpp"
#include "Source/Runtime/Core/Allocator/Public/Allocator.h"
#include "Source/Runtime/DescriptorHeap/Public/Descriptors.h"
#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"

namespace Engine::Rendering
{
	using CandidateTuple = std::tuple<Weak<Engine::Abstracts::ObjectBase>, Weak<Engine::Resources::Material>, aligned_vector<Engine::Graphics::SBs::InstanceSB>>;
	using RenderMap = tbb::concurrent_hash_map<eRenderComponentType, tbb::concurrent_vector<CandidateTuple, u_align_allocator<CandidateTuple>>>;

	void BuildRenderMap(
		const Weak<Scene>& w_scene, RenderMap out_map[SHADER_DOMAIN_MAX], std::atomic<UINT64>& instance_count
	);
	void PremapModelImpl(
		const Weak<Components::ModelRenderer>& model_component, RenderMap out_map[SHADER_DOMAIN_MAX],
		std::atomic<UINT64>& instance_count
	);
	void PremapParticleImpl(
		const Weak<Components::ParticleRenderer>& particle_component, RenderMap out_map[SHADER_DOMAIN_MAX],
		std::atomic<UINT64>& instance_count
	);
}