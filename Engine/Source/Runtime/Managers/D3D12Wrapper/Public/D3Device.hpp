#pragma once
#include <dxgi1_5.h>
#include <Windows.h>
#include <wrl/client.h>
#include <directx/d3d12.h>

#include "Source/Runtime/Allocator/Public/Allocator.h"
#include "Source/Runtime/Abstracts/CoreSingleton/Public/Singleton.hpp"

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

	enum eCommandList
	{
		COMMAND_LIST_PRE_RENDER,
		COMMAND_LIST_RENDER,
		COMMAND_LIST_POST_RENDER,
		COMMAND_LIST_UPDATE,
		COMMAND_LIST_COPY,
		COMMAND_LIST_COMPUTE,
		COMMAND_LIST_COUNT
	};

	enum eCommandNativeType
	{
		COMMAND_NATIVE_DIRECT = D3D12_COMMAND_LIST_TYPE_DIRECT,
		COMMAND_NATIVE_COPY = D3D12_COMMAND_LIST_TYPE_COPY,
		COMMAND_NATIVE_COMPUTE = D3D12_COMMAND_LIST_TYPE_COMPUTE
	};
}

namespace Engine::Managers
{
	using FrameIndex = uint32_t;
	using namespace DirectX;
	using Microsoft::WRL::ComPtr;
	using namespace Engine::Graphics;

	const std::unordered_map<std::wstring, eShaderType> g_shader_type_map = {
		{L"vs", SHADER_VERTEX}, {L"ps", SHADER_PIXEL}, {L"gs", SHADER_GEOMETRY},
		{L"cs", SHADER_COMPUTE}, {L"hs", SHADER_HULL}, {L"ds", SHADER_DOMAIN}
	};

	const std::unordered_map<eShaderType, std::string> g_shader_target_map = {
		{SHADER_VERTEX, "vs_5_0"}, {SHADER_PIXEL, "ps_5_0"},
		{SHADER_GEOMETRY, "gs_5_0"}, {SHADER_COMPUTE, "cs_5_0"},
		{SHADER_HULL, "hs_5_0"}, {SHADER_DOMAIN, "ds_5_0"}
	};

	class D3Device final : public Abstracts::Singleton<D3Device, HWND>
	{
	public:
		D3Device(SINGLETON_LOCK_TOKEN)
			: Singleton() {}

		void            Initialize(HWND hWnd) override;
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

		static float GetAspectRatio();

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

		[[nodiscard]] ID3D12Device* GetDevice() const
		{
			return m_device_.Get();
		}

		[[nodiscard]] HANDLE                      GetSwapchainAwaiter() const;
		[[nodiscard]] ID3D12GraphicsCommandList1* GetCommandList(eCommandList list_enum, UINT64 frame_idx = -1) const;

		[[nodiscard]] ID3D12CommandQueue* GetCommandQueue(eCommandList list) const;
		[[nodiscard]] ID3D12CommandQueue* GetCommandQueue(eCommandTypes type) const;

		[[nodiscard]] UINT64 GetFrameIndex() const
		{
			return m_frame_idx_;
		}

		[[nodiscard]] Weak<CommandPair> AcquireCommandPair(
			const std::wstring& debug_name, UINT64 buffer_idx = -1
		);
		bool IsCommandPairAvailable(UINT64 buffer_idx = -1) const;

		void WaitAndReset(eCommandList list, UINT64 buffer_idx = -1) const;
		void Wait(UINT64 buffer_idx = -1) const;
		void Signal(eCommandTypes type, UINT64 buffer_idx = -1) const;

		void CreateTextureFromFile(
			const std::filesystem::path& file_path,
			ID3D12Resource**             res,
			bool                         generate_mip
		) const;

		void DefaultRenderTarget(const Weak<CommandPair>& w_cmd) const;
		void CopyBackBuffer(const Weak<CommandPair>& w_cmd, ID3D12Resource* resource) const;
		void ClearRenderTarget(bool barrier = true);

		[[nodiscard]] ID3D12DescriptorHeap* GetRTVHeap() const;
		[[nodiscard]] ID3D12DescriptorHeap* GetDSVHeap() const;
		[[nodiscard]] UINT                  GetRTVHeapSize() const;

	private:
		friend struct SingletonDeleter;
		friend struct CommandPair;
		friend struct DescriptorHandler;
		friend class RenderPipeline;
		friend class ToolkitAPI;
		friend class GarbageCollector;

		inline static constexpr eCommandNativeType s_native_target_types[] =
		{
			COMMAND_NATIVE_DIRECT,
			COMMAND_NATIVE_DIRECT,
			COMMAND_NATIVE_DIRECT,
			COMMAND_NATIVE_DIRECT,
			COMMAND_NATIVE_COPY,
			COMMAND_NATIVE_COMPUTE
		};

		inline static constexpr eCommandTypes s_target_types[] =
		{
			COMMAND_TYPE_DIRECT,
			COMMAND_TYPE_DIRECT,
			COMMAND_TYPE_DIRECT,
			COMMAND_TYPE_DIRECT,
			COMMAND_TYPE_COPY,
			COMMAND_TYPE_COMPUTE
		};

		~D3Device() override;

		D3Device() = default;

		void InitializeDevice();
		void InitializeCommandAllocator();
		void InitializeFence();
		void InitializeConsumer();

		void WaitForEventCompletion(UINT64 buffer_idx) const;
		void WaitForCommandsCompletion() const;
		void WaitNextFrame();
		void ConsumeCommands();

		UINT64 GetFenceValue(UINT64 buffer_idx = -1) const;

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


		ComPtr<ID3D12Fence>  m_fence_       = nullptr;
		HANDLE               m_fence_event_ = nullptr;
		std::atomic<UINT64>* m_fence_nonce_;

		std::atomic<UINT64> m_command_ids_ = 0;
		UINT64              m_frame_idx_   = 0;

		std::map<FrameIndex, std::vector<Strong<CommandPair>>>     m_command_pairs_;
		std::array<ComPtr<ID3D12CommandQueue>, COMMAND_TYPE_COUNT> m_command_queues_;

		std::mutex                      m_command_pairs_mutex_;
		pool_queue<Weak<CommandPair>>   m_command_pairs_generated_;
		std::atomic<UINT64>             m_command_pairs_count_;

		std::thread       m_command_consumer_;
		std::atomic<bool> m_command_consumer_running_ = false;

		std::mutex m_command_producer_mutex_;

		XMMATRIX s_world_matrix_      = {};
		Matrix   m_projection_matrix_ = {};
		Matrix   m_ortho_matrix_      = {};
	};
} // namespace Engine::Managers