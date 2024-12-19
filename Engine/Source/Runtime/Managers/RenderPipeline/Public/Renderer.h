#pragma once
#include <atomic>

#include "Source/Runtime/Core/Allocator/Public/Allocator.h"
#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/Core/Singleton/Public/Singleton.hpp"
#include "Source/Runtime/Core/StructuredBuffer.h"
#include "RenderTask.h"

namespace Engine 
{
	using RenderInstanceIndex = uint64_t;
	using CandidateTuple = std::tuple<Weak<Engine::Abstracts::ObjectBase>, Weak<Engine::Resources::Material>, aligned_vector<Engine::Graphics::SBs::InstanceSB>>;
	using RenderMapValueType = tbb::concurrent_vector<CandidateTuple, u_align_allocator<CandidateTuple>>;
	using RenderMap = tbb::concurrent_hash_map<RenderInstanceIndex, RenderMapValueType>;
}

namespace Engine::Managers
{
	class RENDERPIPELINE_API Renderer : public Abstracts::Singleton<Renderer>
	{
	public:
		explicit Renderer(SINGLETON_LOCK_TOKEN)
			: Singleton(),
			  m_b_ready_(false) {}

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;
		void PostUpdate(const float& dt) override;
		void Initialize() override;

		RAWPOINTER void RegisterRenderInstance(RenderInstanceTask* task);
		RAWPOINTER void RegisterRenderPass(RenderPassTask* task);
		RAWPOINTER void RegisterRenderPassPrerequisite(RenderPassPrerequisiteTask* task);

		void RenderPass(
			const float                                        dt,
			const bool                                         shader_bypass,
			const eShaderDomain                                domain,
			const Graphics::SBs::LocalParamSB&                 local_param_sb,
			const aligned_vector<RenderPassPrerequisiteTask*>& additional_task, const ObjectPredication& predication
		);

		[[nodiscard]] bool Ready() const;
		[[nodiscard]] uint64_t GetInstanceCount() const;

	private:
		friend struct SingletonDeleter;
		friend class RayTracer;
		~Renderer() override;
		
		bool m_b_ready_;
		RAWPOINTER aligned_vector<RenderInstanceTask*> m_render_instance_tasks_;
		RAWPOINTER aligned_vector<RenderPassTask*> m_render_pass_tasks_;
		RAWPOINTER aligned_vector<RenderPassPrerequisiteTask*> m_render_prerequisite_tasks_;
		std::atomic<uint64_t> m_instance_count_;
		RenderMap m_render_candidates_[SHADER_DOMAIN_MAX];
	};
}
