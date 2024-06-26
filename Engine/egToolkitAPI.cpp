#include "pch.h"
#include "egToolkitAPI.h"
#include "egCamera.h"
#include "egSceneManager.hpp"
#include <DescriptorHeap.h>

namespace Engine::Manager::Graphics
{
  ToolkitAPI::~ToolkitAPI()
  {
    m_audio_engine_->update();
    m_audio_engine_->release();
  }

  void ToolkitAPI::Initialize()
  {
    m_descriptor_heap_ = std::make_unique<DescriptorHeap>
      (GetD3Device().GetDevice(), 1);

    m_states_              = std::make_unique<CommonStates>(GetD3Device().GetDevice());
    m_resource_upload_batch_ = std::make_unique<ResourceUploadBatch>(GetD3Device().GetDevice());
    m_render_target_state_ = std::make_unique<RenderTargetState>(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D24_UNORM_S8_UINT);

    m_sprite_pipeline_state_ = std::make_unique<SpriteBatchPipelineStateDescription>(*m_render_target_state_.get());

    m_resource_upload_batch_->Begin();

    m_sprite_batch_          = std::make_unique<SpriteBatch>
      (GetD3Device().GetDevice(), *m_resource_upload_batch_.get(), *m_sprite_pipeline_state_.get());

    m_resource_upload_batch_->End(GetD3Device().GetCommandQueue(COMMAND_LIST_UPDATE));

    m_sprite_batch_->SetViewport(GetRenderPipeline().GetViewport());
    
    m_primitive_batch_ = std::make_unique<PrimitiveBatch<VertexPositionColor>>(GetD3Device().GetDevice());

    m_graphics_memory_ = std::make_unique<GraphicsMemory>(GetD3Device().GetDevice());

    m_geometric_primitive_ = GeometricPrimitive::CreateTeapot();

    m_effect_pipeline_state_ = std::make_unique<EffectPipelineStateDescription>
      (
       &VertexPositionColor::InputLayout,
       CommonStates::Opaque,
       CommonStates::DepthDefault,
       CommonStates::CullNone,
       *m_render_target_state_.get(),
       D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);

    m_basic_effect_ = std::make_unique<BasicEffect>(GetD3Device().GetDevice(), EffectFlags::VertexColor, *m_effect_pipeline_state_.get());

    m_basic_effect_->SetProjection(GetD3Device().GetProjectionMatrix());

    FMOD::DX::ThrowIfFailed(System_Create(&m_audio_engine_));

    FMOD::DX::ThrowIfFailed(m_audio_engine_->init(32, FMOD_INIT_NORMAL, nullptr));

    FMOD::DX::ThrowIfFailed
      (
       m_audio_engine_->createChannelGroup("Master", &m_master_channel_group_)
      );

    m_master_channel_group_->setVolume(0.5f);

  }

  void ToolkitAPI::PreUpdate(const float& dt)
  {
  }

  void ToolkitAPI::Update(const float& dt)
  {
    m_audio_engine_->update();
  }

  void ToolkitAPI::PreRender(const float& dt) {  }

  void ToolkitAPI::Render(const float& dt) {}

  void ToolkitAPI::PostRender(const float& dt)
  {
    m_sprite_batch_->SetViewport(GetRenderPipeline().GetViewport());

    ID3D12DescriptorHeap* heaps[] = { m_descriptor_heap_->Heap(), m_states_->Heap() };

    const auto& s_cmd = GetD3Device().AcquireCommandPair(L"Toolkit Sprite Render").lock();

    s_cmd->SoftReset();

    m_sprite_batch_->Begin
    (
        s_cmd->GetList(), 
        SpriteSortMode_Deferred
    );

    GetD3Device().DefaultRenderTarget(s_cmd);
    GetRenderPipeline().DefaultScissorRect(s_cmd);
    GetRenderPipeline().DefaultViewport(s_cmd);
    s_cmd->GetList()->SetDescriptorHeaps(2, heaps);

    for (const auto& callback : m_sprite_batch_callbacks_)
    {
      callback();
    }

    m_sprite_batch_->End();

    s_cmd->FlagReady();

    const auto& p_cmd = GetD3Device().AcquireCommandPair(L"Toolkit Primitive Render").lock();

    p_cmd->SoftReset();

    m_basic_effect_->Apply(p_cmd->GetList());

    if (const auto& scene = GetSceneManager().GetActiveScene().lock())
    {
      if (const auto& cam = scene->GetMainCamera().lock())
      {
        m_basic_effect_->SetView(cam->GetViewMatrix());
        m_basic_effect_->SetProjection(cam->GetProjectionMatrix());
      }
    }

    GetD3Device().DefaultRenderTarget(p_cmd);
    GetRenderPipeline().DefaultScissorRect(p_cmd);
    GetRenderPipeline().DefaultViewport(p_cmd);
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

    m_graphics_memory_->Commit(GetD3Device().GetCommandQueue(COMMAND_TYPE_DIRECT));
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

  SpriteBatch* ToolkitAPI::GetSpriteBatch() const { return m_sprite_batch_.get(); }

  CommonStates* ToolkitAPI::GetCommonStates() const { return m_states_.get(); }

  PrimitiveBatch<VertexPositionColor>* ToolkitAPI::GetPrimitiveBatch() const { return m_primitive_batch_.get(); }

  DescriptorHeap* ToolkitAPI::GetDescriptorHeap() const
  {
    return m_descriptor_heap_.get();
  }

  void ToolkitAPI::LoadSound(FMOD::Sound** sound, const std::string& path) const
  {
    FMOD::DX::ThrowIfFailed
      (
       m_audio_engine_->createSound
       (
        path.c_str(), FMOD_3D | FMOD_3D_LINEARROLLOFF, nullptr,
        sound
       )
      );
  }

  void ToolkitAPI::PlaySound(
    FMOD::Sound*       sound, const FMOD_VECTOR& pos,
    const FMOD_VECTOR& vel,
    FMOD::Channel**    channel
  ) const
  {
    FMOD::DX::ThrowIfFailed
      (
       m_audio_engine_->playSound
       (
        sound, m_master_channel_group_, false, channel
       )
      );

    if (pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f && vel.x == 0.0f &&
        vel.y == 0.f && vel.z == 0.f) { return; }

    FMOD::DX::ThrowIfFailed((*channel)->set3DAttributes(&pos, &vel));
    FMOD::DX::ThrowIfFailed((*channel)->set3DSpread(360.0f));
  }

  void ToolkitAPI::StopSound(FMOD::Sound* sound, FMOD::Channel** channel) const
  {
    FMOD::DX::ThrowIfFailed
      (
       m_audio_engine_->playSound
       (
        sound, m_master_channel_group_, true, channel
       )
      );
    FMOD::DX::ThrowIfFailed((*channel)->set3DAttributes(nullptr, nullptr));
  }

  void ToolkitAPI::Set3DListener(
    const FMOD_VECTOR& position,
    const FMOD_VECTOR& velocity,
    const FMOD_VECTOR& forward,
    const FMOD_VECTOR& up
  ) const
  {
    FMOD::DX::ThrowIfFailed
      (
       m_audio_engine_->set3DListenerAttributes
       (
        0, &position, &velocity, &forward, &up
       )
      );
  }

  void ToolkitAPI::FrameBegin()
  {
    /*const auto& buffer_heap = GetRenderPipeline().GetBufferHeap();

    const CD3DX12_CPU_DESCRIPTOR_HANDLE buffer_handle(buffer_heap->GetCPUDescriptorHandleForHeapStart());

    m_previous_handle_ = buffer_handle;

    GetD3Device().GetDevice()->CopyDescriptorsSimple
    (
        1, 
        buffer_handle, 
        m_descriptor_heap_->GetCpuHandle(0),
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
    );*/
  }

  void ToolkitAPI::FrameEnd() const
  {
    /*GetD3Device().ExecuteSubDirectCommandList();

    const auto& buffer_heap = GetRenderPipeline().GetBufferHeap();

    const CD3DX12_CPU_DESCRIPTOR_HANDLE buffer_handle(buffer_heap->GetCPUDescriptorHandleForHeapStart());

    GetD3Device().GetDevice()->CopyDescriptorsSimple
    (
        1, 
        buffer_handle, 
        m_previous_handle_,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
    );*/
  }
} // namespace Engine::Manager::Graphics
