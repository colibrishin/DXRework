#include "../Public/ToolkitAPI.h"
#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"
#include "Source/Runtime/Core/Objects/Camera/Public/Camera.h"
#include "Source/Runtime/Core/SceneManager/Public/SceneManager.hpp"
#include "Source/Runtime/D3D12GraphicInterface/Public/D3D12GraphicInterface.h"

namespace Engine::Managers
{
	ToolkitAPI::~ToolkitAPI()
	{
	}

	void ToolkitAPI::Initialize()
	{
		auto& gi = reinterpret_cast<D3D12GraphicInterface&>(g_graphic_interface.GetInterface());
		auto dev = static_cast<ID3D12Device2*>(gi.GetNativeInterface());
		
		m_descriptor_heap_ = std::make_unique<DirectX::DescriptorHeap>(dev, 1);

		m_states_                = std::make_unique<DirectX::CommonStates>(dev);
		m_resource_upload_batch_ = std::make_unique<DirectX::ResourceUploadBatch>(dev);
		m_render_target_state_   = std::make_unique<DirectX::RenderTargetState>(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D24_UNORM_S8_UINT);

		m_sprite_pipeline_state_ = std::make_unique<DirectX::SpriteBatchPipelineStateDescription>(*m_render_target_state_.get());

		m_resource_upload_batch_->Begin();

		m_sprite_batch_ = std::make_unique<DirectX::SpriteBatch>(dev, *m_resource_upload_batch_.get(), *m_sprite_pipeline_state_.get());

		m_resource_upload_batch_->End(gi.GetCommandTask().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));

		m_primitive_batch_ = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(dev);

		m_graphics_memory_ = std::make_unique<DirectX::GraphicsMemory>(dev);

		m_geometric_primitive_ = DirectX::GeometricPrimitive::CreateTeapot();

		m_effect_pipeline_state_ = std::make_unique<DirectX::EffectPipelineStateDescription>
				(
				 &DirectX::VertexPositionColor::InputLayout,
				 DirectX::CommonStates::Opaque,
				 DirectX::CommonStates::DepthDefault,
				 DirectX::CommonStates::CullNone,
				 *m_render_target_state_.get(),
				 D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE
				);

		m_basic_effect_ = std::make_unique<DirectX::BasicEffect>(dev, DirectX::EffectFlags::VertexColor, *m_effect_pipeline_state_.get());

		m_basic_effect_->SetProjection(gi.GetProjectionMatrix());
	}

	void ToolkitAPI::PreUpdate(const float& dt) { }

	void ToolkitAPI::Update(const float& dt)
	{
	}

	void ToolkitAPI::PreRender(const float& dt) { }

	void ToolkitAPI::Render(const float& dt) {}

	void ToolkitAPI::PostRender(const float& dt)
	{
		m_sprite_batch_->SetViewport(reinterpret_cast<const D3D12_VIEWPORT&>(RenderPipeline::GetInstance().GetViewport()));

		ID3D12DescriptorHeap*                    heaps[]     = {m_descriptor_heap_->Heap(), m_states_->Heap()};
		auto&                                    gi          = reinterpret_cast<D3D12GraphicInterface&>(g_graphic_interface.GetInterface());
		const GraphicInterfaceContextReturnType& s_context   = gi.GetNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, false, L"Toolkit Render");
		const GraphicInterfaceContextPrimitive&  s_primitive = s_context.GetPointers();
		auto                                     s_cmd       = static_cast<CommandPair*>(s_primitive.commandList);

		s_cmd->SoftReset();
		m_sprite_batch_->Begin(s_cmd->GetList(), DirectX::SpriteSortMode_Deferred);

		gi.SetDefaultGraphicPipeline(&s_primitive);
		gi.SetViewport(&s_primitive, RenderPipeline::GetInstance().GetViewport());
		s_cmd->GetList()->SetDescriptorHeaps(2, heaps);

		for (const auto& callback : m_sprite_batch_callbacks_)
		{
			callback();
		}

		m_sprite_batch_->End();
		s_cmd->FlagReady();
		
		const GraphicInterfaceContextReturnType& p_context   = gi.GetNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, false, L"Toolkit Render");
		const GraphicInterfaceContextPrimitive&  p_primitive = p_context.GetPointers();
		auto                                     p_cmd       = static_cast<CommandPair*>(p_primitive.commandList);
		p_cmd->SoftReset();

		m_basic_effect_->Apply(p_cmd->GetList());

		if (const auto& scene = SceneManager::GetInstance().GetActiveScene().lock())
		{
			if (const auto& cam = scene->GetMainCamera().lock())
			{
				m_basic_effect_->SetView(cam->GetViewMatrix());
				m_basic_effect_->SetProjection(cam->GetProjectionMatrix());
			}
		}

		gi.SetDefaultGraphicPipeline(&p_primitive);
		gi.SetViewport(&p_primitive, RenderPipeline::GetInstance().GetViewport());
		p_cmd->GetList()->SetDescriptorHeaps(2, heaps);

		m_primitive_batch_->Begin(p_cmd->GetList());

		for (const auto& callback : m_primitive_batch_callbacks_)
		{
			callback();
		}

		m_primitive_batch_->End();
		p_cmd->FlagReady();
		
		m_sprite_batch_callbacks_.clear();
		m_primitive_batch_callbacks_.clear();

		m_graphics_memory_->Commit(gi.GetCommandTask().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));
	}

	void ToolkitAPI::FixedUpdate(const float& dt) { }

	void ToolkitAPI::PostUpdate(const float& dt) { }

	void ToolkitAPI::AppendSpriteBatch(const std::function<void()>& callback)
	{
		m_sprite_batch_callbacks_.push_back(callback);
	}

	void ToolkitAPI::AppendPrimitiveBatch(const std::function<void()>& callback)
	{
		m_primitive_batch_callbacks_.push_back(callback);
	}

	DirectX::SpriteBatch* ToolkitAPI::GetSpriteBatch() const
	{
		return m_sprite_batch_.get();
	}

	DirectX::CommonStates* ToolkitAPI::GetCommonStates() const
	{
		return m_states_.get();
	}

	DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* ToolkitAPI::GetPrimitiveBatch() const
	{
		return m_primitive_batch_.get();
	}

	DirectX::DescriptorHeap* ToolkitAPI::GetDescriptorHeap() const
	{
		return m_descriptor_heap_.get();
	}
} // namespace Engine::Manager::Graphics
