#pragma once
#include <memory>
#include "Source/Runtime/Core/ConstantBuffer.h"
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/Core/Singleton/Public/Singleton.hpp"
#include "Source/Runtime/Core/ConstantBuffer.h"
#include "Source/Runtime/Resources/Texture/Public/Texture.h"

namespace Engine 
{
	struct RENDERPIPELINE_API PrimitiveTexture 
	{
		virtual ~PrimitiveTexture() = default;
		virtual void Generate(const Weak<Resources::Texture>& texture) = 0;
		virtual void LoadFromFile(const Weak<Resources::Texture>& texture, const std::filesystem::path& path) = 0;
		virtual void SaveAsFile(const std::filesystem::path& path) = 0;
		virtual void Map(void* src_ptr, const size_t stride, const size_t count) = 0;
		void UpdateDescription(const Weak<Resources::Texture>& texture, const GenericTextureDescription& description);
		[[nodiscard]] void* GetPrimitiveTexture() const;

	protected:
		void SetPrimitiveTexture(void* texture);

	private:
		void* m_texture_ = nullptr;
	};

	struct RENDERPIPELINE_API GraphicPrimitiveShader
	{
	public:
		virtual             ~GraphicPrimitiveShader() = default;
		virtual void        Generate(const Weak<Resources::Shader>& shader, void* pipeline_signature) = 0;
		[[nodiscard]] void* GetGraphicPrimitiveShader() const;

	private:
		void* m_shader_ = nullptr;
	};

	struct RENDERPIPELINE_API ComputePrimitiveShader
	{
	public:
		virtual             ~ComputePrimitiveShader() = default;
		virtual void        Generate(const Weak<Resources::ComputeShader>& shader, void* pipeline_signature) = 0;
		virtual void		Dispatch(const Weak<Resources::ComputeShader>& shader) = 0;
		[[nodiscard]] void* GetComputePrimitiveShader() const;

	private:
		void* m_shader_ = nullptr;
	};

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

	struct RENDERPIPELINE_API ShaderRenderPrerequisiteTask : RenderPassPrerequisiteTask
	{
		void SetShader(const GraphicPrimitiveShader* shader);
		void SetPipelineSignature(void* signature);

		[[nodiscard]] const GraphicPrimitiveShader* GetShader() const;
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
	using namespace Engine::Graphics;

	class RENDERPIPELINE_API RenderPipeline final : public Abstracts::Singleton<RenderPipeline>
	{
	private:
		struct RENDERPIPELINE_API TempParamTicket
		{
			TempParamTicket(const CBs::ParamCB& previousParam)
				: previousParam(previousParam) {}

			~TempParamTicket()
			{
				Managers::RenderPipeline::GetInstance().m_param_buffer_ = previousParam;
			}

		private:
			const CBs::ParamCB previousParam;
		};

	public:
		explicit RenderPipeline(SINGLETON_LOCK_TOKEN) {}

		void Initialize() override;
		void PreRender(const float& dt) override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void Render(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostRender(const float& dt) override;
		void PostUpdate(const float& dt) override;

		void SetPerspectiveMatrix(const CBs::PerspectiveCB& matrix);

		template <typename T>
		void SetParam(const T& v, const size_t slot)
		{
			m_param_buffer_.SetParam(slot, v);
			m_param_cb_task_->SetData(&m_param_buffer_, 1);
		}

		[[nodiscard]] TempParamTicket&& SetParam(const ParamBase& param)
		{
			return {m_param_buffer_};
		}

		template <typename T> requires (std::is_base_of_v<ViewportRenderPrerequisiteTask, T>)
		void SetViewportPrerequisiteTask()
		{
			m_viewport_task_ = std::make_unique<T>();
		}

		template <typename T> requires (std::is_base_of_v<ShaderRenderPrerequisiteTask, T>)
		void SetShaderPrerequisiteTask()
		{
			m_graphics_shader_task_ = std::make_unique<T>();
		}

		template <typename T> requires (std::is_base_of_v<PipelineRenderPrerequisiteTask, T>)
		void SetPipelinePrerequisiteTask()
		{
			m_pipeline_task_ = std::make_unique<T>();
		}

		template <typename T> requires (std::is_base_of_v<ConstantBufferRenderPrerequisiteTask<CBs::PerspectiveCB>, T>)
		void SetPerspectiveConstantBufferPrerequisiteTask()
		{
			m_perspective_cb_task_ = std::make_unique<T>();
		}

		template <typename T> requires (std::is_base_of_v<ConstantBufferRenderPrerequisiteTask<CBs::ParamCB>, T>)
		void SetParamConstantBufferPrerequisiteTask()
		{
			m_param_cb_task_ = std::make_unique<T>();
		}

		[[nodiscard]] ViewportRenderPrerequisiteTask* GetDefaultViewportPrerequisiteTask() const;
		[[nodiscard]] ShaderRenderPrerequisiteTask* GetShaderRenderPrerequisiteTask(const GraphicPrimitiveShader* shader) const;
		[[nodiscard]] PipelineRenderPrerequisiteTask* GetPipelineRenderPrerequisiteTask() const;
		
		[[nodiscard]] ConstantBufferRenderPrerequisiteTask<CBs::PerspectiveCB>* GetPerspectiveConstantBufferRenderPrerequisiteTask() const;
		[[nodiscard]] ConstantBufferRenderPrerequisiteTask<CBs::ParamCB>* GetParamConstantBufferRenderPrerequisiteTask() const;

		void SetPrimitivePipeline(PrimitivePipeline* pipeline);
		[[nodiscard]] PrimitivePipeline* GetPrimitivePipeline() const;

	private:
		friend class ToolkitAPI;
		friend class D3Device;

		friend struct SingletonDeleter;
		RenderPipeline() = default;
		~RenderPipeline() override;

		void PrecompileShaders();
		void InitializeViewport();

		Viewport m_viewport_;

		std::unique_ptr<Engine::PipelineRenderPrerequisiteTask> m_pipeline_task_;
		std::unique_ptr<Engine::ViewportRenderPrerequisiteTask> m_viewport_task_;
		std::unique_ptr<Engine::ShaderRenderPrerequisiteTask> m_graphics_shader_task_;

		std::unique_ptr<Engine::PrimitivePipeline> m_graphics_primitive_pipeline_;
		
		std::unique_ptr<Engine::ConstantBufferRenderPrerequisiteTask<CBs::PerspectiveCB>> m_perspective_cb_task_;
		std::unique_ptr<Engine::ConstantBufferRenderPrerequisiteTask<CBs::ParamCB>> m_param_cb_task_;
		
		Graphics::CBs::PerspectiveCB m_wvp_buffer_;
		Graphics::CBs::ParamCB       m_param_buffer_;
	};
} // namespace Engine::Manager::Graphics
