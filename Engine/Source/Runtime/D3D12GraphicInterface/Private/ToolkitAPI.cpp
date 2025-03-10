#include "ToolkitAPI.h"
#include "RenderPipeline.h"
#include "Camera.h"
#include "SceneManager.h"
#include "D3D12GraphicInterface.h"
#include "DebugDraw.h"
#include "Debugger.h"

namespace Engine::Managers
{
	ToolkitAPI::~ToolkitAPI()
	{
	}

	void ToolkitAPI::Initialize()
	{
		auto& gi = reinterpret_cast<D3D12GraphicInterface&>(g_graphic_accessor.GetInterface());
		auto dev = static_cast<ID3D12Device2*>(gi.GetNativeInterface());
		
		m_descriptor_heap_ = std::make_unique<DirectX::DescriptorHeap>(dev, 1);

		m_states_                = std::make_unique<DirectX::CommonStates>(dev);
		m_resource_upload_batch_ = std::make_unique<DirectX::ResourceUploadBatch>(dev);
		m_render_target_state_   = std::make_unique<DirectX::RenderTargetState>(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D24_UNORM_S8_UINT);
		m_sprite_pipeline_state_ = std::make_unique<DirectX::SpriteBatchPipelineStateDescription>(*m_render_target_state_.get());
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

		m_resource_upload_batch_->Begin();
		m_sprite_batch_ = std::make_unique<DirectX::SpriteBatch>(dev, *m_resource_upload_batch_.get(), *m_sprite_pipeline_state_.get());
		m_font_ = std::make_unique<DirectX::SpriteFont>
					(
					 dev,
					 *m_resource_upload_batch_,
					 L"consolas.spritefont",
					 GetDescriptorHeap()->GetCpuHandle(0),
					 GetDescriptorHeap()->GetGpuHandle(0)
					);
		
		const auto& token = m_resource_upload_batch_->End(gi.GetCommandTask().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));
		token.wait();

		RegisterDebuggerFunction();
	}

	void ToolkitAPI::PreUpdate(const float dt) { }

	void ToolkitAPI::Update(const float dt) { }

	void ToolkitAPI::PreRender(const float dt) { }

	void ToolkitAPI::Render(const float dt)
	{
		m_sprite_batch_->SetViewport(reinterpret_cast<const D3D12_VIEWPORT&>(RenderPipeline::GetInstance().GetViewport()));

		ID3D12DescriptorHeap*                    heaps[]     = {m_descriptor_heap_->Heap(), m_states_->Heap()};
		auto&                                    gi          = reinterpret_cast<D3D12GraphicInterface&>(g_graphic_accessor.GetInterface());
		const IGraphicContextImpl& s_context   = gi.GetNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, false, L"Toolkit Render");
		const IGraphicContext&  s_primitive = s_context.GetPointers();
		const auto                               s_cmd       = static_cast<CommandPair*>(s_primitive.commandList);

		s_cmd->SoftReset();
		m_sprite_batch_->Begin(s_cmd->GetList(), DirectX::SpriteSortMode_Deferred);

		gi.SetViewport(&s_primitive, RenderPipeline::GetInstance().GetViewport());
		gi.SetDefaultRenderTarget(&s_primitive);
		s_cmd->GetList()->SetDescriptorHeaps(2, heaps);

		for (const auto& callback : m_sprite_batch_callbacks_)
		{
			callback();
		}

		m_sprite_batch_->End();
		s_cmd->FlagReady();
		
		const IGraphicContextImpl& p_context   = gi.GetNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, false, L"Toolkit Render");
		const IGraphicContext&  p_primitive = p_context.GetPointers();
		const auto                               p_cmd       = static_cast<CommandPair*>(p_primitive.commandList);
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

		gi.SetViewport(&p_primitive, RenderPipeline::GetInstance().GetViewport());
		gi.SetDefaultRenderTarget(&p_primitive);
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

	void ToolkitAPI::PostRender(const float dt) {}

	void ToolkitAPI::FixedUpdate(const float dt) { }

	void ToolkitAPI::PostUpdate(const float dt) { }

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

	void ToolkitAPI::RegisterDebuggerFunction()
	{
		Debugger::GetInstance().SetCallback(DEBUG_MSG_LOG, [this](const Message& msg)
		{
			AppendSpriteBatch([&]()
			{
				m_font_->DrawString
				(
				 GetSpriteBatch(), msg.text.c_str(),
				 XMFLOAT2(msg.x, msg.y),
				 msg.color, 0.0f, Vector2::Zero, 0.5f
				);
			});
			
		});

		Debugger::GetInstance().SetCallback(DEBUG_MSG_LINE, [this](const Message& msg)
		{
			AppendPrimitiveBatch([&]()
			{
				DX::DrawRay(GetPrimitiveBatch(), msg.ray_start, msg.ray_end, false, msg.color);	
			});
		});

		Debugger::GetInstance().SetCallback(DEBUG_MSG_RAY, [this](const Message& msg)
		{
			AppendPrimitiveBatch([&]()
			{
				DX::DrawRay(GetPrimitiveBatch(), msg.ray.position, msg.ray.direction, true, msg.color);
			});
		});

		Debugger::GetInstance().SetCallback(DEBUG_MSG_FRUSTUM, [this](const Message& msg)
		{
			AppendPrimitiveBatch([&]()
			{
				DX::Draw(GetPrimitiveBatch(), msg.frustum, msg.color);
			});
		});

		Debugger::GetInstance().SetCallback(DEBUG_MSG_SPHERE, [this](const Message& msg)
		{
			AppendPrimitiveBatch([&]()
			{
				DX::Draw(GetPrimitiveBatch(), msg.sphere, msg.color);
			});
		});

		Debugger::GetInstance().SetCallback(DEBUG_MSG_OBB, [this](const Message& msg)
		{
			AppendPrimitiveBatch([&]()
			{
				DX::Draw(GetPrimitiveBatch(), msg.obb, msg.color);
			});
		});

		Debugger::GetInstance().SetCallback(DEBUG_MSG_AABB, [this](const Message& msg)
		{
			AppendPrimitiveBatch([&]()
			{
				DX::Draw(GetPrimitiveBatch(), msg.aabb, msg.color);
			});
		});
	}
} // namespace Engine::Manager::Graphics
