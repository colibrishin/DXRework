#pragma once
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

namespace Engine
{
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

	struct CORE_API GraphicInterface
	{
		virtual void Initialize() = 0;
		virtual void Shutdown() = 0;
		virtual void WaitForNextFrame() = 0;
		virtual void Present() = 0;

		virtual Matrix GetProjectionMatrix() = 0;
		virtual Matrix GetOrthogonalMatrix() = 0;

		template <typename T>
		Unique<StructuredBufferTypeInterface<T>>&& GetStructuredBuffer() 
		{
			return std::move(std::reinterpret_pointer_cast<StructuredBufferTypeInterface<T>>(GetNativeStructuredBuffer()));
		}

		template <typename T>
		Unique<StructuredBufferMemoryPoolBase<T>>&& GetStructuredBufferMemoryPool() 
		{
			rerturn std::move(std::reinterpret_pointer_cast<StructuredBufferMemoryPoolBase<T>>(GetNativeStructuredBufferMemoryPool()));
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

		void CopyRenderTarget(const GraphicInterfaceContextPrimitive* context, Resources::Texture* tex);

	protected:
		virtual Unique<StructuredBufferTypelessBase>&& GetNativeStructuredBuffer() = 0;
	};
}