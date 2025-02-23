#include "Components/Public/ShadowIntersectionComponent.h"
#include "ShadowIntersectionComponent.generated.h"

#include "Shader.h"
#include "ComputeShader.h"
#include "Renderer.h"
#include "Texture2D.h"
#include "Resources/Public/ShadowIntensityComputeShader.h"
#include "ShadowManager.h"
#include "Camera.h"
#include "SceneManager.h"
#include "ShadowTexture.h"
#include "Transform.h"

#include "Resources/Public/IntensityTexture.h"
#include "Resources/Public/IntensityPositionTexture.h"
#include "Resources/Public/ShadowMaskTexture.h"

using namespace Engine;

void ShadowIntersectionComponent::Initialize()
{
    Component::Initialize();
    
    UINT idx = 0;
	for (auto& tex : m_shadow_texs_)
	{
		tex = Resources::ShadowTexture::Create("IntensityTestShadow" + std::to_string( idx ));
		idx++;
	}

    idx = 0;
	for (auto& tex : m_intensity_test_texs_)
	{
        tex = IntensityTexture::Create( "IntensityTest" + std::to_string( idx ), "" );
        ++idx;
	}

    idx = 0;
	for (auto& tex : m_intensity_position_texs_)
	{
        tex = IntensityPositionTexture::Create( "IntensityPositionTest" + std::to_string( idx ), "" );
        ++idx;
	}

	idx = 0;
    for (auto& tex : m_shadow_mask_texs_)
	{
        tex = ShadowMaskTexture::Create( "ShadowMaskTexture" + std::to_string( idx ), "" );
        ++idx;
	}

    GraphicInterface& gi = GraphicInterfaceAccessor::GetInterface();
	m_sb_light_table_ = boost::make_shared<decltype(m_sb_light_table_)::element_type>(gi.GetStructuredBuffer<LightTableSB>());

	const auto& context = gi.GetNewContext(0, false, L"Shadow Intersection");
    const auto& primitive = context.GetPointers();

    primitive.commandList->SoftReset();
	m_sb_light_table_->Create(&primitive, CFG_CASCADE_SHADOW_COUNT, nullptr);
	primitive.commandList->FlagReady();
}

void ShadowIntersectionComponent::PreUpdate( const float dt )
{}

void ShadowIntersectionComponent::Update( const float dt )
{
    Vector3 position;
    Vector3 dir;

    if (const auto& scene = Managers::SceneManager::GetInstance().GetActiveScene().lock())
    {
        if (const auto& camera = scene->GetMainCamera().lock())
        {
            position = camera->GetComponent<Components::Transform>().lock()->GetWorldPosition();
            dir      = camera->GetComponent<Components::Transform>().lock()->Forward();
        }
    }

    for (const auto& [key, bbox] : m_shadow_bbox_)
    {
        float dist = 0.f;

        if (bbox.Intersects(position, dir, dist))
        {
            Managers::Debugger::GetInstance().Draw(bbox, { 0.f, 1.f, 1.f, 1.f });
        }
        else
        {
            Managers::Debugger::GetInstance().Draw(bbox, { 1.f, 0.f, 0.f, 1.f });
        }
    }

    m_shadow_bbox_.clear();
}

void ShadowIntersectionComponent::FixedUpdate( const float dt )
{
    
}

void ShadowIntersectionComponent::OnSerialized()
{
    
}

eComponentUpdatePriorities ShadowIntersectionComponent::GetUpdatePriority() const
{
    return COM_PRIORITY_RENDER;
}
