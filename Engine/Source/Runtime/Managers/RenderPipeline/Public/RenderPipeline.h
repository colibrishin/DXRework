#pragma once
#include <memory>
#include "Source/Runtime/Core/ConstantBuffer.h"
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "Source/Runtime/Core/Singleton/Public/Singleton.hpp"
#include "RenderTask.h"

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

		template <typename T> requires (std::is_base_of_v<GraphicInterface, T>) 
		void SetGraphicInterface() 
		{
			if (!m_graphic_interface_) 
			{
				m_graphic_interface_ = std::make_unique<T>();
				m_graphic_interface_->Initialize();
			}
		}

		[[nodiscard]] GraphicInterface& GetInterface() const;

	private:
		friend struct SingletonDeleter;
		RenderPipeline() = default;
		~RenderPipeline() override;

		void PrecompileShaders();
		void InitializeViewport();

		Viewport m_viewport_;

		std::unique_ptr<GraphicInterface> m_graphic_interface_;

		Unique<StructuredBufferTypeBase<Graphics::CBs::PerspectiveCB>> m_wvp_buffer_;
		Unique<StructuredBufferTypeBase<Graphics::CBs::ParamCB>> m_param_buffer_;

		Graphics::CBs::PerspectiveCB m_wvp_buffer_;
		Graphics::CBs::ParamCB       m_param_buffer_;
	};
} // namespace Engine::Manager::Graphics
