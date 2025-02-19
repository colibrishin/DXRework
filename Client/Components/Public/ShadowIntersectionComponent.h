#pragma once
#include "RenderComponent.h"
#include "ComputeShaders/Public/ShadowIntensityComputeShader.h"
#include "ShadowManager.h"

#include "ShadowIntersectionComponent.generated.h"

class IntensityPositionTexture;
class IntensityTexture;
class ShadowMaskTexture;

ECLASS(component=client, serialize)
class ENGINE_CLIENT_API ShadowIntersectionComponent : public Engine::Components::RenderComponent
{
    GENERATE_BODY
public:
    void Initialize() override;
    void                               PreUpdate( const float dt ) override;
    void                               Update( const float dt ) override;
    void                               FixedUpdate( const float dt ) override;
    void                               OnSerialized() override;
    Engine::eComponentUpdatePriorities GetUpdatePriority() const override;

private:
    friend struct ShadowIntersectionRenderTask;
    using RenderComponent::RenderComponent;

    std::map<std::pair<UINT, UINT>, BoundingBox> m_shadow_bbox_;

    Engine::Strong<Engine::StructuredBufferTypeProxy<LightTableSB>> m_sb_light_table_;

    Engine::Strong<Engine::Resources::ShadowTexture> m_shadow_texs_[ CFG_MAX_DIRECTIONAL_LIGHT ];
    Engine::Strong<ShadowMaskTexture>                m_shadow_mask_texs_[ CFG_MAX_DIRECTIONAL_LIGHT ];

    std::array<Engine::Resources::Texture*, CFG_MAX_DIRECTIONAL_LIGHT> m_shadow_texs_raw_{};
    std::array<Engine::Resources::Texture*, CFG_MAX_DIRECTIONAL_LIGHT> m_shadow_mask_texs_raw_{};

    Engine::Strong<IntensityTexture>         m_intensity_test_texs_[ CFG_MAX_DIRECTIONAL_LIGHT ];
    Engine::Strong<IntensityPositionTexture> m_intensity_position_texs_[ CFG_MAX_DIRECTIONAL_LIGHT ];
};
