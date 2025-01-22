#pragma once
#include <dxgi1_5.h>
#include <Windows.h>
#include <wrl/client.h>
#include <directx/d3d12.h>

#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "Source/Runtime/Core/Allocator/Public/Allocator.h"
#include "Source/Runtime/Core/Singleton/Public/Singleton.hpp"
#include "Source/Runtime/CommandPair/Public/CommandPair.h"

#if WITH_DEBUG
#include "dxgidebug.h"
#endif

#include <array>
#include <unordered_map>
#include <string>
#include <map>

namespace Engine 
{
	static std::atomic<bool> g_raytracing;
}

namespace Engine::Managers
{
	using FrameIndex = uint32_t;
	using namespace DirectX;
	using Microsoft::WRL::ComPtr;
	using namespace Engine::Graphics;

	class D3D12WRAPPER_API D3Device final : public Abstracts::Singleton<D3Device>
	{
	public:
		D3Device(SINGLETON_LOCK_TOKEN)
			: Singleton() {}

		void            Initialize() override;
		ID3D12Resource* GetRenderTarget(UINT64 frame_idx);

		static void DEBUG_MEMORY()
		{
#if WITH_DEBUG
			HMODULE hModule = GetModuleHandleW(L"dxgidebug.dll");
			auto    DXGIGetDebugInterfaceFunc =
				reinterpret_cast<decltype(DXGIGetDebugInterface)*>(
					GetProcAddress(hModule, "DXGIGetDebugInterface"));

			IDXGIDebug* pDXGIDebug;
			DXGIGetDebugInterfaceFunc(IID_PPV_ARGS(&pDXGIDebug));
			pDXGIDebug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_DETAIL);
#endif // WITH_DEBUG
		}

		static float      GetAspectRatio();

		Weak<CommandPair> AcquireCommandPair(const D3D12_COMMAND_LIST_TYPE type, const std::wstring& debug_name);

		std::vector<std::pair<D3D12_INPUT_ELEMENT_DESC, std::string>> GenerateInputDescription(
			ID3DBlob* blob
		);

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostRender(const float& dt) override;
		void PostUpdate(const float& dt) override;

		const Matrix& GetProjectionMatrix() const
		{
			return m_projection_matrix_;
		}

		const Matrix& GetOrthogonalMatrix() const
		{
			return m_ortho_matrix_;
		}

		[[nodiscard]] ID3D12Device2* GetDevice() const
		{
			return m_device_.Get();
		}

		[[nodiscard]] HANDLE                      GetSwapchainAwaiter() const;
		[[nodiscard]] UINT64 GetFrameIndex() const
		{
			return m_frame_idx_;
		}

		bool IsCommandPairAvailable(UINT64 buffer_idx = -1) const;
		void Wait(UINT64 buffer_idx) const;

		void DefaultRenderTarget(ID3D12GraphicsCommandList4* cmd) const;
		void CopyBackBuffer(ID3D12GraphicsCommandList4* w_cmd, ID3D12Resource* resource) const;
		void ClearRenderTarget(bool barrier = true);

		[[nodiscard]] ID3D12DescriptorHeap* GetRTVHeap() const;
		[[nodiscard]] ID3D12DescriptorHeap* GetDSVHeap() const;
		[[nodiscard]] UINT                  GetRTVHeapSize() const;
		[[nodiscard]] ID3D12CommandQueue*   GetCommandQueue(const D3D12_COMMAND_LIST_TYPE type) const;

		void WaitForCommandsCompletion() const;

	private:
		friend struct SingletonDeleter;
		friend struct CommandPair;
		friend struct DescriptorHandler;
		friend class RenderPipeline;
		friend class ToolkitAPI;

		~D3Device() override;

		D3Device() = default;

		void InitializeDevice();
		void InitializeConsumer();

		void WaitNextFrame();

		HWND m_hwnd_ = nullptr;

		ComPtr<ID3D12Device2> m_device_ = nullptr;

		UINT s_video_card_memory_        = 0;
		UINT s_refresh_rate_numerator_   = 0;
		UINT s_refresh_rate_denominator_ = 0;

		DXGI_ADAPTER_DESC s_video_card_desc_ = {};

		ComPtr<IDXGISwapChain4> m_swap_chain_ = nullptr;

		std::vector<ComPtr<ID3D12Resource>> m_render_targets_;
		ComPtr<ID3D12DescriptorHeap>        m_rtv_heap_;
		UINT                                m_rtv_heap_size_;

		ComPtr<ID3D12Resource>       m_depth_stencil_;
		ComPtr<ID3D12DescriptorHeap> m_dsv_heap_;

		CommandPairTask      m_command_pair_task_;
		std::thread          m_command_consumer_;

		ComPtr<ID3D12Fence>  m_fence_       = nullptr;
		HANDLE               m_fence_event_ = nullptr;
		std::atomic<UINT64>* m_fence_nonce_;

		UINT64              m_frame_idx_   = 0;

		std::mutex m_command_producer_mutex_;

		XMMATRIX s_world_matrix_      = {};
		Matrix   m_projection_matrix_ = {};
		Matrix   m_ortho_matrix_      = {};
	};
} // namespace Engine::Managers