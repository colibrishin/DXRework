#pragma once
#include "Client.h"
#include "clIntensityPositionTexture.h"
#include "clIntensityTexture.h"
#include "clIntersectionCompute.h"
#include "clShadowMaskTexture.h"
#include "egScript.h"
#include "egShadowTexture.h"
#include "egStructuredBuffer.hpp"

namespace Client::Scripts
{
  class ShadowIntersectionScript : public Script
  {
  public:
	  CLIENT_SCRIPT_T(ShadowIntersectionScript, SCRIPT_T_SHADOW)

    explicit ShadowIntersectionScript(const WeakObjectBase& owner)
      : Script(SCRIPT_T_SHADOW, owner),
        m_viewport_() {}

	  ~ShadowIntersectionScript() override = default;

    void Initialize() override;
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PostUpdate(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void FirstPass(
      const float& dt, 
      const Weak<CommandPair>& cmd, 
      const size_t shadow_slot, 
      const StrongLayer& lights,
      UINT&        instance_idx
    );
    void SecondPass(
      const float&                                 dt,
      const Weak<CommandPair>&                     cmd,
      const std::vector<Graphics::SBs::LightVPSB>& light_vps,
      const StrongScene&                           scene, 
      const StrongLayer& lights, 
      UINT& instance_idx
    );
    void ThirdPass(const Weak<CommandPair>& cmd, const StrongLayer& lights);
    void PostRender(const float& dt) override;

  protected:
    void OnCollisionEnter(const WeakCollider& other) override;
    void OnCollisionContinue(const WeakCollider& other) override;
    void OnCollisionExit(const WeakCollider& other) override;

  private:
    SERIALIZE_DECL
    SCRIPT_CLONE_DECL

    ShadowIntersectionScript();

    ComPtr<ID3D12DescriptorHeap> m_srv_heap_;
    ComPtr<ID3D12DescriptorHeap> m_multiple_dsv_heap_;

    D3D12_VIEWPORT m_viewport_;
    D3D12_RECT m_scissor_rect_;

    std::map<std::pair<UINT, UINT>, BoundingBox> m_shadow_bbox_;

    Strong<Graphics::StructuredBuffer<ComputeShaders::IntersectionCompute::LightTableSB>> m_sb_light_table_;

    Engine::Resources::ShadowTexture m_shadow_texs_[g_max_lights];
    Client::Resource::ShadowMaskTexture m_shadow_mask_texs_[g_max_lights];

    Client::Resource::IntensityTexture m_intensity_test_texs_[g_max_lights];
    Client::Resource::IntensityPositionTexture m_intensity_position_texs_[g_max_lights];

    DescriptorContainer m_shadow_heaps_;
    StrongDescriptorPtr m_shadow_third_pass_heap_;

    InstanceBufferContainer m_instance_buffers_;

    std::vector<Strong<Graphics::StructuredBuffer<Graphics::SBs::LocalParamSB>>> m_first_other_pass_local_params_;
    std::vector<Strong<Graphics::StructuredBuffer<Graphics::SBs::LocalParamSB>>> m_first_self_pass_local_params_;
    std::vector<Strong<Graphics::StructuredBuffer<Graphics::SBs::LocalParamSB>>> m_second_pass_local_params_;
    std::vector<Graphics::StructuredBuffer<Graphics::SBs::LocalParamSB>> m_third_compute_local_param_;

    StrongTexture2D m_tmp_shadow_depth_;
    StrongComputeShader m_intersection_compute_;
    StrongShader m_shadow_shader_;
    StrongShader m_intensity_test_shader_;
  };
}

REGISTER_TYPE(Engine::Script, Client::Scripts::ShadowIntersectionScript)
BOOST_CLASS_EXPORT_KEY(Client::Scripts::ShadowIntersectionScript)