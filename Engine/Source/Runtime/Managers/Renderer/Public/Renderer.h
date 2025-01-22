#pragma once
#include <atomic>
#include "Source/Runtime/Core/Singleton/Public/Singleton.hpp"
#include "Source/Runtime/Core/Allocator/Public/Allocator.h"
#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "RenderTask.h"

namespace Engine::Managers
{
	class RENDERER_API Renderer : public Abstracts::Singleton<Renderer>
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
		RAWPOINTER void RegisterRenderPassPrequisite(RenderPassPrequisiteTask* task);

		void RenderPass(const float dt, const eShaderDomain domain);

		bool Ready() const;
		uint64_t GetInstanceCount() const;

	private:
		friend struct SingletonDeleter;
		friend class RayTracer;
		~Renderer() override;
		
		bool m_b_ready_;
		RAWPOINTER aligned_vector<RenderInstanceTask*> m_render_instance_tasks_;
		RAWPOINTER aligned_vector<RenderPassTask*> m_render_pass_tasks_;
		RAWPOINTER aligned_vector<RenderPassPrequisiteTask*> m_render_prequisite_tasks_;
		std::atomic<uint64_t> m_instance_count_;
		RenderMap m_render_candidates_[SHADER_DOMAIN_MAX];
	};
}
