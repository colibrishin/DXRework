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
				GetInstance().m_param_buffer_ = previousParam;
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
			const GraphicInterfaceContextReturnType& context = g_graphic_interface.GetInterface().GetNewContext(0, false, L"Pipeline Parameter setting");
			const GraphicInterfaceContextPrimitive& primitive = context.GetPointers();
			primitive.commandList->SoftReset();
			m_param_buffer_cb_->SetData(&primitive, 1, &m_param_buffer_);
			primitive.commandList->FlagReady();
		}

		[[nodiscard]] TempParamTicket&& SetParam(const ParamBase& param)
		{
			return {m_param_buffer_};
		}

	private:
		friend struct SingletonDeleter;
		RenderPipeline() = default;
		~RenderPipeline() override;

		void PrecompileShaders();
		void InitializeViewport();

		Viewport m_viewport_;

		Unique<IStructuredBufferType<CBs::PerspectiveCB>> m_wvp_buffer_cb_;
		Unique<IStructuredBufferType<CBs::ParamCB>> m_param_buffer_cb_;

		CBs::PerspectiveCB m_wvp_buffer_;
		CBs::ParamCB       m_param_buffer_;
	};
} // namespace Engine::Manager::Graphics
