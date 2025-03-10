#pragma once
#include "RenderComponent.h"
#if CLIENT || WITH_EDITOR
#include "Resources/Public/ShadowIntensityComputeShader.h"
#include "ShadowManager.h"
#endif

#include "ShadowIntersectionComponent.generated.h"

#if CLIENT || WITH_EDITOR
class IntensityPositionTexture;
class IntensityTexture;
class ShadowMaskTexture;
#endif

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
    using RenderComponent::RenderComponent;

#if CLIENT || WITH_EDITOR
    friend struct ShadowIntersectionRenderTask;
    std::map<std::pair<UINT, UINT>, BoundingBox> m_shadow_bbox_;

    Engine::Strong<Engine::StructuredBufferTypeProxy<LightTableSB>> m_sb_light_table_;

    Engine::Strong<Engine::Resources::ShadowTexture> m_shadow_texs_[ CFG_MAX_DIRECTIONAL_LIGHT ];
    Engine::Strong<ShadowMaskTexture>                m_shadow_mask_texs_[ CFG_MAX_DIRECTIONAL_LIGHT ];

    std::array<Engine::Resources::Texture*, CFG_MAX_DIRECTIONAL_LIGHT> m_shadow_texs_raw_{};
    std::array<Engine::Resources::Texture*, CFG_MAX_DIRECTIONAL_LIGHT> m_shadow_mask_texs_raw_{};

    Engine::Strong<IntensityTexture>         m_intensity_test_texs_[ CFG_MAX_DIRECTIONAL_LIGHT ];
    Engine::Strong<IntensityPositionTexture> m_intensity_position_texs_[ CFG_MAX_DIRECTIONAL_LIGHT ];
#endif
};
