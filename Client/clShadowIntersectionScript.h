#pragma once
#include "Client.h"
#include "egGlobal.h"
#include "egObject.hpp"
#include "egScript.h"
#include "egShadowTexture.h"
#include "egStructuredBuffer.hpp"
#include "clIntensityTexture.h"

namespace Client::Scripts
{
  class ShadowIntersectionScript : public Script
  {
  public:
	  CLIENT_SCRIPT_T(ShadowIntersectionScript, SCRIPT_T_SHADOW)

    explicit ShadowIntersectionScript(const WeakObject& owner)
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
    SERIALIZER_ACCESS
    ShadowIntersectionScript();

    D3D11_VIEWPORT m_viewport_;

    Graphics::StructuredBuffer<Graphics::SBs::LightVPSB> m_sb_light_vp_;


    Engine::Resources::ShadowTexture m_shadow_texs_[g_max_lights];
    Client::Resource::IntensityTexture m_intensity_test_texs_[g_max_lights];
    StrongTexture2D m_shadow_depth_;

    StrongMaterial m_shadow_material_;
    StrongMaterial m_intensity_test_material_;
  };
}

BOOST_CLASS_EXPORT_KEY(Client::Scripts::ShadowIntersectionScript)