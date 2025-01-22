#pragma once
#include <atomic>

#include "Source/Runtime/Core/Allocator/Public/Allocator.h"
#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/CoreSingleton/Public/Singleton.h"
#include "RenderTask.h"

#include "Renderer.generated.h"

namespace Engine::Managers
{
	ECLASS()
	class ENGINE_RENDERPIPELINE_API Renderer : public Abstracts::Singleton<Renderer>
	{
		GENERATE_BODY
	public:
		explicit Renderer(SINGLETON_LOCK_TOKEN)
			: Singleton(),
			  m_b_ready_(false) {}

		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void PostRender(const float dt) override;
		void PostUpdate(const float dt) override;
		void Initialize() override;
		
		void RegisterRenderInstance(const std::wstring_view name, RenderInstanceTask* task);
		void RegisterRenderPass(const std::wstring_view name, RenderPassTask* task);
		void UnregisterRenderInstance(const std::wstring_view name);
		void UnregisterRenderPass(const std::wstring_view name);

		void RenderPass(
			float dt,
			bool shader_bypass,
			eShaderDomain domain,
			const Graphics::SBs::LocalParamSB& local_param_sb,
			const ObjectPredication& predication, const ContextSetupFunction& prerender_predicate, const ContextSetupFunction&
			postrender_predicate
		) const;

		[[nodiscard]] bool Ready() const;
		[[nodiscard]] uint64_t GetInstanceCount() const;

	private:
		friend struct SingletonDeleter;
		friend class RayTracer;
		~Renderer() override;
		
		bool m_b_ready_;
		std::unordered_map<std::wstring, Unique<RenderInstanceTask>> m_render_instance_tasks_;
		std::unordered_map<std::wstring, Unique<RenderPassTask>> m_render_pass_tasks_;
		std::atomic<uint64_t> m_instance_count_;
		RenderMap m_render_candidates_[SHADER_DOMAIN_MAX];
	};
}
