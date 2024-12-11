#pragma once
#include <atomic>

#include "Source/Runtime/Resources/Shader/Public/Shader.hpp"

#include "Source/Runtime/Core/Allocator/Public/Allocator.h"
#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/Core/Singleton/Public/Singleton.hpp"
#include "Source/Runtime/Core/StructuredBuffer.h"

namespace Engine
{
	using RenderInstanceIndex = uint64_t;
	using CandidateTuple = std::tuple<Weak<Engine::Abstracts::ObjectBase>, Weak<Engine::Resources::Material>, aligned_vector<Engine::Graphics::SBs::InstanceSB>>;
	using RenderMapValueType = tbb::concurrent_vector<CandidateTuple, u_align_allocator<CandidateTuple>>;
    using RenderMap = tbb::concurrent_hash_map<RenderInstanceIndex, RenderMapValueType>;

	struct CORE_API RenderInstanceTask
	{
		virtual      ~RenderInstanceTask() = default;
		virtual void Run(Scene const* scene, const RenderMapValueType* render_map, std::atomic<uint64_t>& instance_count) = 0;
		virtual void Cleanup(const RenderMapValueType* render_map) = 0;
	};

	struct RenderPassTask;

	struct CORE_API RenderPassPrerequisiteTask
	{
		virtual      ~RenderPassPrerequisiteTask() = default;
		virtual void Run(RenderPassTask* task_context) = 0;
		virtual void Cleanup(RenderPassTask* task_context) = 0;
	};

	struct CORE_API RenderPassTask
	{
		virtual      ~RenderPassTask() = default;
		virtual void Run(
			const float dt,
			const bool shader_bypass,
			const eShaderDomain domain, 
			RenderMap const* domain_map,
			const Graphics::SBs::LocalParamSB& local_param,
			const std::atomic<uint64_t>& instance_count, 
			RenderPassPrerequisiteTask* const* prerequisite, 
			const size_t prerequisite_count,
			const ObjectPredication& predicate) = 0;

		virtual void Cleanup() = 0;
	};

	struct CORE_API ViewportRenderPrerequisiteTask : RenderPassPrerequisiteTask
	{
		void SetViewport(const Viewport& viewport);
		[[nodiscard]] Viewport GetViewport() const;

	private:
		Engine::Viewport m_viewport_{};
	};

	struct CORE_API PipelineRenderPrerequisiteTask : RenderPassPrerequisiteTask
	{
		void SetPrimitivePipeline(PrimitivePipeline* pipeline);

	protected:
		[[nodiscard]] PrimitivePipeline* GetPrimitivePipeline() const;

	private:
		PrimitivePipeline* m_pipeline_ = nullptr;
	};

	struct CORE_API ShaderRenderPrerequisiteTask : RenderPassPrerequisiteTask
	{
		void SetShader(const GraphicPrimitiveShader* shader);
		void SetPipelineSignature(void* signature);

		[[nodiscard]] GraphicPrimitiveShader* GetShader() const;
		[[nodiscard]] void* GetPipelineSignature() const;

	private:
		const GraphicPrimitiveShader* m_shader_ = nullptr;
		void* m_pipeline_signature_ = nullptr;
	};

	template <typename T>
	struct StructuredBufferRenderPrerequisiteTask : RenderPassPrerequisiteTask
	{
		void SetData(const T* data, const size_t count)
		{
			if (data)
			{
				m_ptr_ = data;
				m_count_ = count;
			}
		}

	protected:
		void GetData(T** out_ptr, size_t& out_count)
		{
			*out_ptr = m_ptr_;
			out_count = m_count_;
		}

		[[nodiscard]] bool IsDirty() const
		{
			return m_dirty_;
		}

		void FlipDirty()
		{
			m_dirty_ = true;
		}

	private:
		bool m_dirty_ = true;

		const T* m_ptr_ = nullptr;
		size_t m_count_ = 1;
	};

	template <typename T>
	struct ConstantBufferRenderPrerequisiteTask : RenderPassPrerequisiteTask
	{
		void SetData(const T* data, const size_t count)
		{
			if (data)
			{
				m_ptr_ = data;
				m_count_ = count;
			}
		}

	protected:
		void GetData(T** out_ptr, size_t& out_count)
		{
			*out_ptr = m_ptr_;
			out_count = m_count_;
		}
		
		[[nodiscard]] bool IsDirty() const
		{
			return m_dirty_;
		}

		void FlipDirty()
		{
			m_dirty_ = true;
		}

	private:
		bool m_dirty_ = true;

		const T* m_ptr_ = nullptr;
		size_t m_count_ = 1;
	};
}

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
