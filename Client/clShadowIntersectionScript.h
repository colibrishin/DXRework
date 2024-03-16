#pragma once
#include "Client.h"
#include "clIntensityPositionTexture.h"
#include "egGlobal.h"
#include "egObject.hpp"
#include "egScript.h"
#include "egShadowTexture.h"
#include "egStructuredBuffer.hpp"
#include "clIntensityTexture.h"
#include "clIntersectionCompute.h"
#include "clShadowMaskTexture.h"

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
    void PostRender(const float& dt) override;

  private:
    SERIALIZE_DECL
    SCRIPT_CLONE_DECL

    ShadowIntersectionScript();

    D3D11_VIEWPORT m_viewport_;

    std::map<std::pair<UINT, UINT>, BoundingBox> m_shadow_bbox_;

    Graphics::StructuredBuffer<Graphics::SBs::LightVPSB> m_sb_light_vp_;
    Graphics::StructuredBuffer<ComputeShaders::IntersectionCompute::LightTableSB> m_sb_light_table_;

    Engine::Resources::ShadowTexture m_shadow_texs_[g_max_lights];
    Client::Resource::ShadowMaskTexture m_shadow_mask_texs_[g_max_lights];

    Client::Resource::IntensityTexture m_intensity_test_texs_[g_max_lights];
    Client::Resource::IntensityPositionTexture m_intensity_position_texs_[g_max_lights];

    StrongTexture2D m_tmp_shadow_depth_;
    StrongComputeShader m_intersection_compute_;

    StrongMaterial m_shadow_material_;
    StrongMaterial m_intensity_test_material_;
  };
}

BOOST_CLASS_EXPORT_KEY(Client::Scripts::ShadowIntersectionScript)