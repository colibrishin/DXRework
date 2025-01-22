#pragma once
#include <directx/d3dx12.h>
#include <directx/d3d12.h>

#include "Source/Runtime/CommandPair/Public/CommandPair.h"
#include "Source/Runtime/ThrowIfFailed/Public/ThrowIfFailed.h"
#include "Source/Runtime/Managers/D3D12Wrapper/Public/D3Device.hpp"

// Static constant buffer type, this should be added to every constant buffer
#define CB_T(enum_val) static constexpr eCBType cbtype = enum_val;
// Static raytracing constant buffer type, this should be added to every constant buffer
#define RT_CB_T(enum_val) static constexpr eRaytracingCBType cbtype = enum_val;

namespace Engine
{
	enum eCBType : uint8_t;
	enum eRaytracingCBType :uint8_t;

	namespace Graphics::CBs 
	{
		struct PerspectiveCB
		{
			CB_T(CB_TYPE_WVP)

				Matrix world;
			Matrix view;
			Matrix projection;

			Matrix invView;
			Matrix invProj;
			Matrix invVP;

			Matrix reflectView;
		};
	}

	template <typename T>
	struct which_cb
	{
		static constexpr eCBType value = T::cbtype;
	};
}

namespace Engine::Graphics
{
	// Creates a constant buffer for only current back buffer.
	template <typename T>
	class ConstantBuffer
	{
	public:
		ConstantBuffer();

		void Create(const T* src_data)
		{
			const auto& cmd = Managers::D3Device::GetInstance().AcquireCommandPair(L"ConstantBuffer Initialization").lock();

			cmd->SoftReset();

			const auto& default_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			const auto& cb_desc      = CD3DX12_RESOURCE_DESC::Buffer(m_alignment_size_);

			DX::ThrowIfFailed
					(
					 Managers::D3Device::GetInstance().GetDevice()->CreateCommittedResource
					 (
					  &default_heap,
					  D3D12_HEAP_FLAG_NONE,
					  &cb_desc,
					  D3D12_RESOURCE_STATE_COPY_DEST,
					  nullptr,
					  IID_PPV_ARGS(m_buffer_.GetAddressOf())
					 )
					);

			const auto         gen_type_name = std::string(typeid(T).name());
			const auto         type_name     = std::wstring(gen_type_name.begin(), gen_type_name.end());
			const std::wstring buffer_name   = type_name + L" Constant Buffer";

			DX::ThrowIfFailed(m_buffer_->SetName(buffer_name.c_str()));

			const auto& upload_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			const auto& buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(m_alignment_size_);

			DX::ThrowIfFailed
					(
					 Managers::D3Device::GetInstance().GetDevice()->CreateCommittedResource
					 (
					  &upload_heap,
					  D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
					  &buffer_desc,
					  D3D12_RESOURCE_STATE_GENERIC_READ,
					  nullptr,
					  IID_PPV_ARGS(m_upload_buffer_.GetAddressOf())
					 )
					);

			const std::wstring upload_buffer_name = type_name + L" Constant Buffer Upload Buffer";

			DX::ThrowIfFailed(m_upload_buffer_->SetName(upload_buffer_name.c_str()));

			if (src_data != nullptr)
			{
				char* data = nullptr;

				DX::ThrowIfFailed(m_upload_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&data)));
				std::memcpy(data, src_data, sizeof(T));
				m_upload_buffer_->Unmap(0, nullptr);

				cmd->GetList()->CopyResource(m_buffer_.Get(), m_upload_buffer_.Get());
			}

			const auto& cb_trans = CD3DX12_RESOURCE_BARRIER::Transition
					(
					 m_buffer_.Get(),
					 D3D12_RESOURCE_STATE_COPY_DEST,
					 D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
					);

			cmd->GetList()->ResourceBarrier(1, &cb_trans);

			const D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc
			{
				.BufferLocation = m_buffer_->GetGPUVirtualAddress(),
				.SizeInBytes = m_alignment_size_
			};

			cmd->FlagReady();

			constexpr D3D12_DESCRIPTOR_HEAP_DESC cbv_heap_desc
			{
				.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				.NumDescriptors = 1,
				.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
				.NodeMask = 0
			};

			DX::ThrowIfFailed
					(
					 Managers::D3Device::GetInstance().GetDevice()->CreateDescriptorHeap
					 (&cbv_heap_desc, IID_PPV_ARGS(m_cpu_cbv_heap_.GetAddressOf()))
					);

			Managers::D3Device::GetInstance().GetDevice()->CreateConstantBufferView
					(
					 &cbv_desc,
					 m_cpu_cbv_heap_->GetCPUDescriptorHandleForHeapStart()
					);

			m_b_dirty_ = false;
		}

		void SetData(const T* src_data)
		{
			if (src_data != nullptr)
			{
				m_data_ = *src_data;
			}

			m_b_dirty_ = true;
		}

		T GetData() const
		{
			return m_data_;
		}

		void Bind(const Weak<CommandPair>& w_cmd, const DescriptorPtr& w_heap)
		{
			if (const auto& cmd = w_cmd.lock())
			{
				Bind(cmd->GetList(), w_heap);
			}
		}

		void Bind(ID3D12GraphicsCommandList* cmd, const DescriptorPtr& w_heap)
		{
			if (m_b_dirty_)
			{
				const auto& copy_trans = CD3DX12_RESOURCE_BARRIER::Transition
						(
						 m_buffer_.Get(),
						 D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
						 D3D12_RESOURCE_STATE_COPY_DEST
						);

				char* data = nullptr;

				DX::ThrowIfFailed(m_upload_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&data)));

				std::memcpy(data, &m_data_, sizeof(T));

				m_upload_buffer_->Unmap(0, nullptr);

				const auto& cb_trans = CD3DX12_RESOURCE_BARRIER::Transition
						(
						 m_buffer_.Get(),
						 D3D12_RESOURCE_STATE_COPY_DEST,
						 D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
						);

				cmd->ResourceBarrier(1, &copy_trans);
				cmd->CopyResource(m_buffer_.Get(), m_upload_buffer_.Get());
				cmd->ResourceBarrier(1, &cb_trans);

				m_b_dirty_ = false;
			}

			const auto& heap = w_heap.lock();

			if (heap == nullptr)
			{
				return;
			}

			heap->SetConstantBuffer(m_cpu_cbv_heap_->GetCPUDescriptorHandleForHeapStart(), which_cb<T>::value);
		}

		[[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const
		{
			return m_buffer_->GetGPUVirtualAddress();
		}

		[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const
		{
			return m_cpu_cbv_heap_->GetCPUDescriptorHandleForHeapStart();
		}

	private:
		T                            m_data_;
		bool                         m_b_dirty_;
		ComPtr<ID3D12DescriptorHeap> m_cpu_cbv_heap_;

		ComPtr<ID3D12Resource> m_upload_buffer_;
		ComPtr<ID3D12Resource> m_buffer_;
		UINT                   m_alignment_size_;
	};

	template <typename T>
	ConstantBuffer<T>::ConstantBuffer()
		: m_b_dirty_(false)
	{
		static_assert(std::is_standard_layout_v<T>, "Constant buffer type must be a POD type");

		m_alignment_size_ = (sizeof(T) + 255) & ~255;
	}
}
