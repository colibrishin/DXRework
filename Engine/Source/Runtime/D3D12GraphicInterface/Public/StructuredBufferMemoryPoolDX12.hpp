#pragma once
#include "StructuredBufferDX12.hpp"
#include "Source/Runtime/Core/StructuredBuffer.h"
#include "Source/Runtime/Core/SIMDExtension/Public/SIMDExtension.hpp"

namespace Engine::Graphics
{
	template <typename SBType>
	class StructuredBufferMemoryPool : StructuredBufferMemoryPoolBase<SBType>
	{
	public:
		StructuredBufferMemoryPool()
			: m_allocated_size_(0),
			  m_used_size_(0),
			  m_read_offset_(0) { }

		~StructuredBufferMemoryPool() { }

		void resize(const size_t size) override
		{
			Update(nullptr, size);
		}

		StructuredBufferTypeBase<SBType>& get()
		{
			return GetBaseResource()[m_read_offset_];
		}

		void advance() override
		{
			StructuredBufferMemoryPoolBase<SBType>::advance();

			if (m_read_offset_ >= m_allocated_size_)
			{
				resize(static_cast<size_t>(m_allocated_size_ * 1.5f));
			}
		}

		void Update(const SBType* src_data, size_t count) override
		{
			if (count == 0)
			{
				count = 1;
			}

			UpdateSizeIfNeeded(count);

			if (!src_data)
			{
				return;
			}

			Copy(src_data, 0, count);
			m_used_size_ = count;
		}

		void UpdatePartial(const SBType* src_data, const size_t offset, size_t count)
		{
			if (count == 0)
			{
				return;
			}

			UpdateSizeIfNeeded(count);

			if (!src_data)
			{
				return;
			}

			Copy(src_data, offset, count);
			m_used_size_ += count;
		}

		[[nodiscard]] auto& GetBaseResource()
		{
			return m_resource_;
		}

		[[nodiscard]] const auto& GetBaseResource() const
		{
			return m_resource_;
		}

	private:
		std::vector<Unique<StructuredBufferTypeInterface<SBType>>> m_resource_;

		void UpdateSizeIfNeeded(const size_t count)
		{
			if (m_allocated_size_ < count)
			{
				const auto& delta  = count - m_resource_.size();
				size_t      end_it = m_resource_.size();
				m_resource_.resize(count);

				GraphicInterface& gi = Managers::RenderPipeline::GetInstance().GetInterface();
				Unique<StructuredBufferTypeInterface<SBType>> sb = gi.GetStructuredBuffer<SBType>();
				const GraphicInterfaceContextReturnType& context = gi.GetNewContext(static_cast<uint8_t>(D3D12_COMMAND_LIST_TYPE_DIRECT), false, L"Structured Buffer Memory pool resizing");
				const GraphicInterfaceContextPrimitive& primitive = context.GetPointers();

				primitive.commandList->SoftReset();

				for (; end_it < count; ++end_it)
				{
					m_resource_[end_it]->SetData(primmitive, 1, nullptr);
				}

				primitive.commandList->FlagReady();

				m_allocated_size_ = count;
			}
		}

		void Copy(const SBType* src_data, const size_t offset, const size_t count)
		{
			if (m_resource_.size() < count)
			{
				throw std::logic_error("Memory pool is not allocated enough size");
			}

			const auto& cmd = Managers::D3Device::GetInstance().AcquireCommandPair(D3D12_COMMAND_LIST_TYPE_DIRECT, L"Update").lock();

			for (size_t i = offset; i < count; ++i)
			{
				m_resource_[i].SetData(cmd->GetList(), 1, src_data);
			}
		}
	};

	template <size_t Alignment, D3D12_HEAP_TYPE HeapProperty, D3D12_RESOURCE_FLAGS ResourceFlags, D3D12_RESOURCE_STATES
	          ResourceState>
	class GraphicMemoryPool
	{
	public:
		GraphicMemoryPool()
			: m_allocated_size_(0),
			  m_used_size_(0) { }

		~GraphicMemoryPool() { }

		void Update(const void* src_data, const size_t type_size, size_t count)
		{
			if (count == 0)
			{
				count = 1;
			}

			if (m_allocated_size_ < count)
			{
				InitializeBuffer(m_resource_, type_size, count);
				m_allocated_size_ = count;
			}

			if (!src_data)
			{
				return;
			}

			char*      data          = nullptr;
			const auto src_char_cast = static_cast<const char*>(src_data);

			DX::ThrowIfFailed(m_resource_->Map(0, nullptr, reinterpret_cast<void**>(&data)));
			for (size_t i = 0; i < count; ++i)
			{
				SIMDExtension::_mm256_memcpy(data + (i * Align(type_size, Alignment)), src_char_cast + (i * type_size), type_size);
			}
			m_resource_->Unmap(0, nullptr);
			m_used_size_ = count;
		}

		void Release()
		{
			m_resource_.Reset();
		}

		[[nodiscard]] ID3D12Resource** GetAddressOf()
		{
			return m_resource_.GetAddressOf();
		}

		[[nodiscard]] ID3D12Resource* GetResource() const
		{
			return m_resource_.Get();
		}

		[[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const
		{
			return GetResource()->GetGPUVirtualAddress();
		}

	private:
		static void InitializeBuffer(ComPtr<ID3D12Resource>& resource, const size_t type_size, const size_t count)
		{
			const auto& heap_desc   = CD3DX12_HEAP_PROPERTIES(HeapProperty);
			const auto& buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(Align(count * type_size, Alignment), ResourceFlags);

			DX::ThrowIfFailed
					(
					 Managers::D3Device::GetInstance().GetDevice()->CreateCommittedResource
					 (
					  &heap_desc,
					  D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
					  &buffer_desc,
					  ResourceState,
					  nullptr,
					  IID_PPV_ARGS(resource.GetAddressOf())
					 )
					);
		}

		ComPtr<ID3D12Resource> m_resource_;
		size_t                 m_allocated_size_;
		size_t                 m_used_size_;
	};
}
