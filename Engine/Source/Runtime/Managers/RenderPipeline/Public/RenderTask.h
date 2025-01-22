#pragma once
#include <memory>
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "RenderType.h"

namespace Engine 
{
	struct RENDERPIPELINE_API RenderInstanceTask
	{
		virtual      ~RenderInstanceTask() = default;
		virtual void Run(Scene const* scene, const RenderMapValueType* render_map, std::atomic<uint64_t>& instance_count) = 0;
		virtual void Cleanup(const RenderMapValueType* render_map) = 0;
	};

	struct RenderPassTask;

	struct RENDERPIPELINE_API RenderPassPrerequisiteTask
	{
		virtual      ~RenderPassPrerequisiteTask() = default;
		virtual void Run(RenderPassTask* task_context) = 0;
		virtual void Cleanup(RenderPassTask* task_context) = 0;
	};

	struct RENDERPIPELINE_API RenderPassTask
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

	struct RENDERPIPELINE_API BufferCopyTask
	{
		virtual ~BufferCopyTask() = default;
		virtual void Run(
			void* src_data,
			const size_t stride,
			const size_t count,
			void* upload_buffer, 
			void* dest_buffer) = 0;

		virtual void Cleanup() = 0;
	};

	struct RENDERPIPELINE_API ViewportRenderPrerequisiteTask : RenderPassPrerequisiteTask
	{
		void SetViewport(const Viewport& viewport);
		[[nodiscard]] Viewport GetViewport() const;

	private:
		Engine::Viewport m_viewport_{};
	};

	struct RENDERPIPELINE_API PipelineRenderPrerequisiteTask : RenderPassPrerequisiteTask
	{
		void SetPrimitivePipeline(PrimitivePipeline* pipeline);

	protected:
		[[nodiscard]] PrimitivePipeline* GetPrimitivePipeline() const;

	private:
		PrimitivePipeline* m_pipeline_ = nullptr;
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