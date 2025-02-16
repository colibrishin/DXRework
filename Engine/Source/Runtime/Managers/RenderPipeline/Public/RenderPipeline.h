#pragma once
#include <memory>
#include "Source/Runtime/Core/ConstantBuffer.h"
#include "Source/Runtime/CoreSingleton/Public/Singleton.h"
#include "RenderTask.h"

#include "IModule.h"

#include "RenderPipeline.generated.h"

namespace Engine::Managers
{
	using namespace Engine::Graphics;

	ECLASS()
	class ENGINE_RENDERPIPELINE_API RenderPipeline final : public Abstracts::Singleton<RenderPipeline>
	{
		GENERATE_BODY
	private:
		struct ENGINE_RENDERPIPELINE_API TempParamTicket
		{
			TempParamTicket(const CBs::ParamCB& previousParam)
				: previousParam(previousParam) {}

			~TempParamTicket()
			{
				GetInstance().m_param_buffer_ = previousParam;
			}

			TempParamTicket(TempParamTicket&) = delete;
			TempParamTicket operator=(TempParamTicket&) = delete;

		private:
			const CBs::ParamCB previousParam;
		};

	public:
		explicit RenderPipeline(SINGLETON_LOCK_TOKEN) {}

		void Initialize() override;
		void PreRender(const float dt) override;
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void Render(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostRender(const float dt) override;
		void PostUpdate(const float dt) override;

		void SetPerspectiveMatrix(const CBs::PerspectiveCB& matrix);
		void BindConstantBuffers(const GraphicInterfaceContextPrimitive* context) const;

		template <typename T>
		void SetParam(const T& v, const size_t slot)
		{
			m_param_buffer_.SetParam(slot, v);
			ConstantBufferGuard();
			m_param_buffer_cb_.SetData(&m_param_buffer_);
		}

		[[nodiscard]] TempParamTicket SetParam(const ParamBase& param)
		{
			return {m_param_buffer_};
		}

		[[nodiscard]] const Viewport& GetViewport() const;

	private:
		friend struct SingletonDeleter;
		RenderPipeline() = default;
		~RenderPipeline() override;

		void ConstantBufferGuard();

		void InitializeViewport();

		Viewport m_viewport_;

		ConstantBufferTypeProxy<CBs::PerspectiveCB> m_wvp_buffer_cb_;
		ConstantBufferTypeProxy<CBs::ParamCB> m_param_buffer_cb_;

		CBs::PerspectiveCB m_wvp_buffer_;
		CBs::ParamCB       m_param_buffer_;
	};
} // namespace Engine::Manager::Graphics
