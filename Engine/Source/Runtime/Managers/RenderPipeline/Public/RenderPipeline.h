#pragma once
#include <memory>
#include "Source/Runtime/Core/ConstantBuffer.h"

#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/Core/Singleton/Public/Singleton.hpp"
#include "Source/Runtime/Managers/Renderer/Public/Renderer.h"

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
