#pragma once
#include "Source/Runtime/Core/Singleton/Public/Singleton.hpp"

#if defined(USE_DX12)
#include <directxtk12/BufferHelpers.h>
#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include "Source/Runtime/DescriptorHeap/Public/Descriptors.h"
#include "Source/Runtime/ConstantBufferDX12/Public/ConstantBufferDX12.hpp"
#endif

namespace Engine::Managers
{
	using namespace Engine::Graphics;

	class RenderPipeline final : public Abstracts::Singleton<RenderPipeline>
	{
	private:
		struct TempParamTicket
		{
			TempParamTicket(const CBs::ParamCB& previousParam)
				: previousParam(previousParam) {}

			~TempParamTicket()
			{
				Managers::RenderPipeline::GetInstance().m_param_buffer_ = previousParam;
				Managers::RenderPipeline::GetInstance().m_param_buffer_data_.SetData(&previousParam);
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
			m_param_buffer_data_.SetData(&m_param_buffer_);
		}

		[[nodiscard]] TempParamTicket SetParam(const ParamBase& param)
		{
			return {m_param_buffer_};
		}

		void DefaultViewport(const Weak<CommandPair>& w_cmd) const;
		void DefaultScissorRect(const Weak<CommandPair>& w_cmd) const;
		void DefaultRootSignature(const Weak<CommandPair>& w_cmd) const;

		ID3D12RootSignature* GetRootSignature() const;

		D3D12_VIEWPORT GetViewport() const;
		D3D12_RECT     GetScissorRect() const;

		static void SetPSO(const Weak<CommandPair>& w_cmd, const Strong<Resources::Shader>& Shader);

		[[nodiscard]] DescriptorPtr AcquireHeapSlot();

		UINT GetBufferDescriptorSize() const;
		UINT GetSamplerDescriptorSize() const;

		void BindConstantBuffers(const Weak<CommandPair>& w_cmd, const DescriptorPtr& heap);
		void BindConstantBuffers(ID3D12GraphicsCommandList1* cmd, const DescriptorPtr& heap);

	private:
		friend class ToolkitAPI;
		friend class D3Device;

		friend struct SingletonDeleter;
		RenderPipeline() = default;
		~RenderPipeline() override;

		void PrecompileShaders();
		void InitializeRootSignature();
		void InitializeNullDescriptors();
		void InitializeHeaps();
		void InitializeStaticBuffers();
		void InitializeViewport();

		ComPtr<ID3D12RootSignature> m_root_signature_ = nullptr;
		ComPtr<ID3D12PipelineState> m_pipeline_state_ = nullptr;

		UINT m_buffer_descriptor_size_  = 0;
		UINT m_sampler_descriptor_size_ = 0;

		std::mutex        m_descriptor_mutex_;
		Engine::DescriptorHandler m_descriptor_handler_;

		ComPtr<ID3D12DescriptorHeap> m_null_srv_heap_;
		ComPtr<ID3D12DescriptorHeap> m_null_sampler_heap_;
		ComPtr<ID3D12DescriptorHeap> m_null_cbv_heap_;
		ComPtr<ID3D12DescriptorHeap> m_null_uav_heap_;
		ComPtr<ID3D12DescriptorHeap> m_null_rtv_heap_;
		ComPtr<ID3D12DescriptorHeap> m_null_dsv_heap_;

		D3D12_VIEWPORT m_viewport_{};
		D3D12_RECT     m_scissor_rect_{};

		Graphics::CBs::PerspectiveCB m_wvp_buffer_;
		Graphics::CBs::ParamCB       m_param_buffer_;

		Graphics::ConstantBuffer<CBs::PerspectiveCB> m_wvp_buffer_data_{};
		Graphics::ConstantBuffer<CBs::ParamCB>       m_param_buffer_data_{};

		Strong<Resources::Shader> m_fallback_shader_;
	};
} // namespace Engine::Manager::Graphics
