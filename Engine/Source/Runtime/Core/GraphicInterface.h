#pragma once
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

namespace Engine
{
	enum CORE_API eHeapType
	{
		HEAP_TYPE_DEFAULT	= 1,
		HEAP_TYPE_UPLOAD	= 2,
		HEAP_TYPE_READBACK	= 3,
		HEAP_TYPE_CUSTOM	= 4,
		HEAP_TYPE_GPU_UPLOAD	= 5
	};

	enum CORE_API eHeapFlag
	{
		HEAP_FLAG_NONE	= 0,
		HEAP_FLAG_SHARED	= 0x1,
		HEAP_FLAG_DENY_BUFFERS	= 0x4,
		HEAP_FLAG_ALLOW_DISPLAY	= 0x8,
		HEAP_FLAG_SHARED_CROSS_ADAPTER	= 0x20,
		HEAP_FLAG_DENY_RT_DS_TEXTURES	= 0x40,
		HEAP_FLAG_DENY_NON_RT_DS_TEXTURES	= 0x80,
		HEAP_FLAG_HARDWARE_PROTECTED	= 0x100,
		HEAP_FLAG_ALLOW_WRITE_WATCH	= 0x200,
		HEAP_FLAG_ALLOW_SHADER_ATOMICS	= 0x400,
		HEAP_FLAG_CREATE_NOT_RESIDENT	= 0x800,
		HEAP_FLAG_CREATE_NOT_ZEROED	= 0x1000,
		HEAP_FLAG_TOOLS_USE_MANUAL_WRITE_TRACKING	= 0x2000,
		HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES	= 0,
		HEAP_FLAG_ALLOW_ONLY_BUFFERS	= 0xc0,
		HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES	= 0x44,
		HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES	= 0x84
	};
	
	struct CORE_API Viewport
	{
		float topLeftX;
		float topLeftY;
		float width;
		float height;
		float minDepth;
		float maxDepth;
	};

	struct CORE_API CommandListBase
	{
		virtual ~CommandListBase() = default;
		virtual void SoftReset() = 0;
		virtual void FlagReady(const std::function<void()>& post_function = {}) = 0;
		virtual void Execute() = 0;
	};

	struct CORE_API GraphicHeapBase
	{
		virtual ~GraphicHeapBase() = default;
		virtual void BindGraphic(CommandListBase* cmd) = 0;
	};

	struct CORE_API GraphicInterfaceContextPrimitive
	{
		CommandListBase* commandList;
		GraphicHeapBase* heap;
	};

	struct CORE_API GraphicResourcePrimitive
	{
	public:
		virtual ~GraphicResourcePrimitive() = default;

		template <typename T>
		T* GetResource()
		{
			return static_cast<T*>(m_resource_);
		}

		template <typename T>
		T** GetAddressOf()
		{
			return static_cast<T**>(&m_resource_);
		}

		virtual void SetResource(void* resource)
		{
			m_resource_ = resource;
		}
		
	private:
		void* m_resource_ = nullptr;
	};

	struct CORE_API GraphicInterfaceContextReturnType
	{
		GraphicInterfaceContextReturnType(const Weak<CommandListBase>& cmd, Unique<GraphicHeapBase>&& heap)
		{
			if (const Strong<CommandListBase>& locked = cmd.lock()) 
			{
				commandList = locked;
			}

			this->heap = std::move(heap);
		}

		GraphicInterfaceContextPrimitive GetPointers() const
		{
			return GraphicInterfaceContextPrimitive
			{
				.commandList = this->commandList.get(),
				.heap = this->heap.get()
			};
		}

	private:
		Strong<CommandListBase> commandList;
		Unique<GraphicHeapBase> heap;
	};

	class StructuredBufferTypelessBase
	{
	public:
		virtual ~StructuredBufferTypelessBase() = default;

		virtual void Create(const GraphicInterfaceContextPrimitive* context, const UINT size, const void* initial_data, const size_t stride, const bool uav) = 0;
		virtual void SetData(const GraphicInterfaceContextPrimitive* context, const UINT size, const void* src_data, const size_t stride) = 0;
		virtual void SetDataContainer(const GraphicInterfaceContextPrimitive* context, const UINT size, const void* const* container_ptr, const size_t stride) = 0;
		virtual void GetData(const GraphicInterfaceContextPrimitive* context, const UINT size, void* dst_ptr, const size_t stride) = 0;
		virtual void Clear() = 0;

		virtual void TransitionToSRV(const GraphicInterfaceContextPrimitive* context) = 0;
		virtual void TransitionToUAV(const GraphicInterfaceContextPrimitive* context) = 0;
		virtual void TransitionCommon(const GraphicInterfaceContextPrimitive* context) = 0;

		virtual void CopySRVHeap(const GraphicInterfaceContextPrimitive* context) const = 0;
		virtual void CopyUAVHeap(const GraphicInterfaceContextPrimitive* context) const = 0;
	};

	template <typename T>
	class IStructuredBufferType 
	{
	public:
		virtual ~IStructuredBufferType() = default;

		virtual void Create(const GraphicInterfaceContextPrimitive* context, const UINT size, const T* initial_data, const bool uav) = 0;
		virtual void SetData(const GraphicInterfaceContextPrimitive* context, const UINT size, const T* src_data) = 0;
		virtual void SetDataContainer(const GraphicInterfaceContextPrimitive* context, const UINT size, const T* const* container_ptr) = 0;
		virtual void GetData(const GraphicInterfaceContextPrimitive* context, const UINT size, T* dst_ptr) = 0;
		virtual void Clear() = 0;

		[[nodiscard]] StructuredBufferTypelessBase& GetTypeless()
		{
			return reinterpret_cast<StructuredBufferTypelessBase&>(*this);
		}
	};

	struct CORE_API GraphicInterface
	{
		virtual      ~GraphicInterface() = default;
		
		virtual void Initialize() = 0;
		virtual void Shutdown() = 0;
		virtual void WaitForNextFrame() = 0;
		virtual void Present() = 0;
		
		virtual void* GetNativeInterface() = 0;
		virtual void* GetNativePipeline() = 0;

		virtual Matrix GetProjectionMatrix() = 0;
		virtual Matrix GetOrthogonalMatrix() = 0;

		template <typename T>
		Unique<IStructuredBufferType<T>>&& GetStructuredBuffer() 
		{
			return std::move(std::reinterpret_pointer_cast<IStructuredBufferType<T>>(GetNativeStructuredBuffer()));
		}

		virtual GraphicInterfaceContextReturnType GetNewContext(const int8_t type, bool heap_allocation, const std::wstring_view debug_name) = 0;
		virtual Strong<CommandListBase> GetCommandList(const int8_t type, const std::wstring_view debug_name) = 0;
		virtual Unique<GraphicHeapBase> GetHeap() = 0;

		virtual void SetViewport(const GraphicInterfaceContextPrimitive* context, const Viewport& viewport) = 0;
		virtual void SetDefaultPipeline(const GraphicInterfaceContextPrimitive* context) = 0;

		virtual void Draw(const GraphicInterfaceContextPrimitive* context, Resources::Shape* shape, const UINT instance_count) = 0;
		virtual void Draw(const GraphicInterfaceContextPrimitive* context, Resources::Mesh* mesh, const UINT instance_count) = 0;

		virtual void Bind(const GraphicInterfaceContextPrimitive* context, Resources::Shader* shader) = 0;

		virtual void Bind(const GraphicInterfaceContextPrimitive* context, Resources::Texture* tex, const eBindType bind_type, const UINT slot, const UINT offset) = 0;
		virtual void Unbind(const GraphicInterfaceContextPrimitive* context, Resources::Texture* tex, const eBindType bind_type) = 0;
		virtual void Clear(const GraphicInterfaceContextPrimitive* context, Resources::Texture* tex, const eBindType clear_type) = 0;

		virtual Unique<GraphicResourcePrimitive> CreateBuffer() = 0;
		
		void CopyRenderTarget(const GraphicInterfaceContextPrimitive* context, Resources::Texture* tex);

	protected:
		virtual Unique<StructuredBufferTypelessBase>&& GetNativeStructuredBuffer() = 0;
	};

	struct GraphicInterfaceAccessor
	{
	public:
		template <typename T> requires (std::is_base_of_v<GraphicInterface, T>)
		void SetGraphicInterface()
		{
			if (!m_graphic_interface_)
			{
				m_graphic_interface_ = std::make_unique<T>();
				m_graphic_interface_->Initialize();
			}
		}

		[[nodiscard]] GraphicInterface& GetInterface() const
		{
			return *m_graphic_interface_;
		}

	private:
		Unique<GraphicInterface> m_graphic_interface_{};
	};

	inline GraphicInterfaceAccessor g_graphic_interface{};

	template <typename T>
	class StructuredBufferMemoryPool
	{
	public:
		virtual ~StructuredBufferMemoryPool() = default;

		void    resize(const size_t size)
		{
			Update(nullptr, size);
		}

		IStructuredBufferType<T>& get()
		{
			return *m_resource_[m_read_offset_];
		}

		virtual void advance() 
		{
			++m_read_offset_;

			if (m_read_offset_ >= m_allocated_size_)
			{
				resize(m_allocated_size_ * 1.5f);
			}
		}

		virtual void reset() 
		{
			m_used_size_ = 0;
			m_read_offset_ = 0;
		}

		void Update(const T* src_data, size_t count)
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

	private:
		void UpdateSizeIfNeeded(const size_t count)
		{
			if (m_allocated_size_ < count)
			{
				const auto& delta  = count - m_resource_.size();
				size_t      end_it = m_resource_.size();
				m_resource_.resize(count);

				GraphicInterface& gi = g_graphic_interface.GetInterface();
				const GraphicInterfaceContextReturnType& context = gi.GetNewContext(0, false, L"Structured Buffer Memory pool resizing");
				const GraphicInterfaceContextPrimitive& primitive = context.GetPointers();

				primitive.commandList->SoftReset();

				for (; end_it < count; ++end_it)
				{
					m_resource_[end_it] = std::move(gi.GetStructuredBuffer<T>());
					m_resource_[end_it]->SetData(&primitive, 1, nullptr);
				}

				primitive.commandList->FlagReady();

				m_allocated_size_ = count;
			}
		}

		void Copy(const T* src_data, const size_t offset, const size_t count)
		{
			if (m_resource_.size() < count)
			{
				throw std::logic_error("Memory pool is not allocated enough size");
			}

			GraphicInterface& gi = g_graphic_interface.GetInterface();
			const GraphicInterfaceContextReturnType& context = gi.GetNewContext(0, false, L"Structured Buffer Memory pool copy");
			const GraphicInterfaceContextPrimitive& primitive = context.GetPointers();

			primitive.commandList->SoftReset();
			
			for (size_t i = offset; i < count; ++i)
			{
				m_resource_[i]->SetData(&primitive, 1, src_data);
			}

			primitive.commandList->FlagReady();
		}

		std::vector<Unique<IStructuredBufferType<T>>> m_resource_;
		size_t                                        m_allocated_size_{};
		size_t                                        m_used_size_{};
		size_t                                        m_read_offset_{};
	};

	class CORE_API GraphicMemoryPool
	{
	public:
		GraphicMemoryPool()
			: m_allocated_size_(0),
			  m_used_size_(0) { }

		virtual ~GraphicMemoryPool() { }

		void Update(const void* src_data, size_t count, const size_t stride)
		{
			if (count == 0)
			{
				count = 1;
			}

			if (m_allocated_size_ < count)
			{
				InitializeBuffer(count, stride);
				m_allocated_size_ = count;
			}

			if (!src_data)
			{
				return;
			}

			Map(src_data, count, stride);
				
			m_used_size_ = count;
		}

		virtual void Map(const void* src_data, const size_t count, const size_t stride) = 0;

		void Release()
		{
			m_resource_.reset();
		}

		template <typename T>
		[[nodiscard]] T** GetAddressOf()
		{
			return m_resource_->GetAddressOf<T>();
		}

		template <typename T>
		[[nodiscard]] T* GetResource() const
		{
			return m_resource_->GetResource<T>();
		}

		[[nodiscard]] GraphicResourcePrimitive& GetPrimitive() const
		{
			return *m_resource_;
		}

	private:
		virtual void InitializeBuffer(const size_t count, const size_t stride) = 0;

		Unique<GraphicResourcePrimitive> m_resource_;
		size_t                           m_allocated_size_;
		size_t                           m_used_size_;
	};
}