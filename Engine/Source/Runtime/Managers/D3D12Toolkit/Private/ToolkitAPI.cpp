#include "../Public/ToolkitAPI.h"
#include "Source/Runtime/Managers/D3D12Wrapper/Public/D3Device.hpp"
#include "Source/Runtime/Managers/RenderPipeline/Public/RenderPipeline.h"
#include "Source/Runtime/CommandPair/Public/CommandPair.h"
#include "Source/Runtime/Core/Objects/Camera/Public/Camera.h"
#include "Source/Runtime/Managers/SceneManager/Public/SceneManager.hpp"

#pragma comment(lib, "fmod_vc.lib")

namespace Engine::Managers
{
	ToolkitAPI::~ToolkitAPI()
	{
	}

	void ToolkitAPI::Initialize()
	{
		m_descriptor_heap_ = std::make_unique<DescriptorHeap>
				(Managers::D3Device::GetInstance().GetDevice(), 1);

		m_states_                = std::make_unique<CommonStates>(Managers::D3Device::GetInstance().GetDevice());
		m_resource_upload_batch_ = std::make_unique<ResourceUploadBatch>(Managers::D3Device::GetInstance().GetDevice());
		m_render_target_state_   = std::make_unique<RenderTargetState>
				(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D24_UNORM_S8_UINT);

		m_sprite_pipeline_state_ = std::make_unique<SpriteBatchPipelineStateDescription>(*m_render_target_state_.get());

		m_resource_upload_batch_->Begin();

		m_sprite_batch_ = std::make_unique<SpriteBatch>
				(Managers::D3Device::GetInstance().GetDevice(), *m_resource_upload_batch_.get(), *m_sprite_pipeline_state_.get());

		m_resource_upload_batch_->End(Managers::D3Device::GetInstance().GetCommandQueue(COMMAND_LIST_UPDATE));

		m_sprite_batch_->SetViewport(Managers::RenderPipeline::GetInstance().GetViewport());

		m_primitive_batch_ = std::make_unique<PrimitiveBatch<VertexPositionColor>>(Managers::D3Device::GetInstance().GetDevice());

		m_graphics_memory_ = std::make_unique<GraphicsMemory>(Managers::D3Device::GetInstance().GetDevice());

		m_geometric_primitive_ = GeometricPrimitive::CreateTeapot();

		m_effect_pipeline_state_ = std::make_unique<EffectPipelineStateDescription>
				(
				 &VertexPositionColor::InputLayout,
				 CommonStates::Opaque,
				 CommonStates::DepthDefault,
				 CommonStates::CullNone,
				 *m_render_target_state_.get(),
				 D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE
				);

		m_basic_effect_ = std::make_unique<BasicEffect>
				(Managers::D3Device::GetInstance().GetDevice(), EffectFlags::VertexColor, *m_effect_pipeline_state_.get());

		m_basic_effect_->SetProjection(Managers::D3Device::GetInstance().GetProjectionMatrix());
	}

	void ToolkitAPI::PreUpdate(const float& dt) { }

	void ToolkitAPI::Update(const float& dt)
	{
	}

	void ToolkitAPI::PreRender(const float& dt) { }

	void ToolkitAPI::Render(const float& dt) {}

	void ToolkitAPI::PostRender(const float& dt)
	{
		m_sprite_batch_->SetViewport(Managers::RenderPipeline::GetInstance().GetViewport());

		ID3D12DescriptorHeap* heaps[] = {m_descriptor_heap_->Heap(), m_states_->Heap()};

		const auto& s_cmd = Managers::D3Device::GetInstance().AcquireCommandPair(L"Toolkit Sprite Render").lock();

		s_cmd->SoftReset();

		m_sprite_batch_->Begin
				(
				 s_cmd->GetList(),
				 SpriteSortMode_Deferred
				);

		Managers::D3Device::GetInstance().DefaultRenderTarget(s_cmd);
		Managers::RenderPipeline::GetInstance().DefaultScissorRect(s_cmd);
		Managers::RenderPipeline::GetInstance().DefaultViewport(s_cmd);
		s_cmd->GetList()->SetDescriptorHeaps(2, heaps);

		for (const auto& callback : m_sprite_batch_callbacks_)
		{
			callback();
		}

		m_sprite_batch_->End();

		s_cmd->FlagReady();

		const auto& p_cmd = Managers::D3Device::GetInstance().AcquireCommandPair(L"Toolkit Primitive Render").lock();

		p_cmd->SoftReset();

		m_basic_effect_->Apply(p_cmd->GetList());

		if (const auto& scene = Managers::SceneManager::GetInstance().GetActiveScene().lock())
		{
			if (const auto& cam = scene->GetMainCamera().lock())
			{
				m_basic_effect_->SetView(cam->GetViewMatrix());
				m_basic_effect_->SetProjection(cam->GetProjectionMatrix());
			}
		}

		Managers::D3Device::GetInstance().DefaultRenderTarget(p_cmd);
		Managers::RenderPipeline::GetInstance().DefaultScissorRect(p_cmd);
		Managers::RenderPipeline::GetInstance().DefaultViewport(p_cmd);
		p_cmd->GetList()->SetDescriptorHeaps(2, heaps);

		m_primitive_batch_->Begin(p_cmd->GetList());

		for (const auto& callback : m_primitive_batch_callbacks_)
		{
			callback();
		}

		m_primitive_batch_->End();

		m_sprite_batch_callbacks_.clear();

		m_primitive_batch_callbacks_.clear();

		p_cmd->FlagReady();

		m_graphics_memory_->Commit(Managers::D3Device::GetInstance().GetCommandQueue(COMMAND_TYPE_DIRECT));
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

	SpriteBatch* ToolkitAPI::GetSpriteBatch() const
	{
		return m_sprite_batch_.get();
	}

	CommonStates* ToolkitAPI::GetCommonStates() const
	{
		return m_states_.get();
	}

	PrimitiveBatch<VertexPositionColor>* ToolkitAPI::GetPrimitiveBatch() const
	{
		return m_primitive_batch_.get();
	}

	DescriptorHeap* ToolkitAPI::GetDescriptorHeap() const
	{
		return m_descriptor_heap_.get();
	}
} // namespace Engine::Manager::Graphics
