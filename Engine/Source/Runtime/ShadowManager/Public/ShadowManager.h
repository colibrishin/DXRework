#pragma once

#include "RenderTask.h"
#include "Allocator.h"
#include "StructuredBuffer.h"
#include "Singleton.h"

#include "ShadowManager.generated.h"

namespace Engine::Resources
{
	class Shader;
	class ShadowTexture;
	class ShadowRenderTarget;
}

namespace Engine::Graphics
{
	namespace SBs
	{
		struct ENGINE_SHADOWMANAGER_API LightVPSB
		{
			SB_T(SB_TYPE_LIGHT_VP)

			Matrix  view[CFG_CASCADE_SHADOW_COUNT];
			Matrix  proj[CFG_CASCADE_SHADOW_COUNT];
			Vector4 end_clip_spaces[CFG_CASCADE_SHADOW_COUNT];
		};
	}
}

namespace Engine::Managers
{
	using namespace Engine::Graphics;
	
	constexpr float _placeholder = 0.f;

	ECLASS()
	class ENGINE_SHADOWMANAGER_API ShadowManager : public Abstracts::Singleton<ShadowManager>
	{
		GENERATE_BODY

	private:
		struct Subfrusta
		{
			Vector4 corners[8];
		};

	public:
		explicit ShadowManager(SINGLETON_LOCK_TOKEN)
			: m_viewport_() {}

		void Initialize() override;
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void GetLightVP(const Strong<Scene>& scene, std::vector<SBs::LightVPSB>& current_light_vp);
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void PostRender(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostUpdate(const float dt) override;

		void Reset();

		static void EvalShadowVP(const Weak<Objects::Camera>& ptr_cam, const Vector3& light_dir, SBs::LightVPSB& buffer);
		void BindShadowMaps(const GraphicInterfaceContextPrimitive* context) const;
		void TransitBackShadowMaps(const GraphicInterfaceContextPrimitive* context) const;

        const StructuredBufferTypeProxy<SBs::LightVPSB> &GetLightVPBuffer() const;
        const std::vector<SBs::LightVPSB> &              GetCurrentSceneLightVP() const;

	private:
		friend struct SingletonDeleter;
		~ShadowManager() override;
		
		void InitializeViewport();
		void InitializeShadowBuffer(LocalActorID id);

		void BuildShadowMap(float dt, UINT light_idx) const;
		void ClearShadowMaps(const GraphicInterfaceContextPrimitive* context);

		static void CreateSubfrusta(
			const Matrix& projection, float start, float end,
			Subfrusta&    subfrusta
		);

        Strong<Resources::Shader>                                m_shadow_shader_;
        std::vector<Strong<Resources::ShadowTexture>>            m_shadow_texs_;

        // sub part of the view frustum
        Subfrusta m_subfrusta_[ 3 ];

        StructuredBufferTypeProxy<SBs::LightVPSB> m_light_vp_sb_;
        std::vector<SBs::LightVPSB>               m_current_scene_light_vp_;
        Unique<PrimitiveSampler>                  m_shadow_sampler_;

        Viewport m_viewport_;
	};
} // namespace Engine::Manager::Graphics
