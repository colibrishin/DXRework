#pragma once

#include "Source/Runtime/Core/Allocator/Public/Allocator.h"
#include "Source/Runtime/Resources/Shader/Public/Shader.hpp"
#include "Source/Runtime/CoreSingleton/Public/Singleton.hpp"
#include "Source/Runtime/Resources/ShadowTexture/Public/ShadowTexture.h"
#include "Source/Runtime/Core/StructuredBuffer.h"

#include "RenderTask.h"

namespace Engine::Graphics
{
	namespace SBs
	{
		struct ENGINE_SHADOWMANAGER_API LightSB
		{
			SB_T(SB_TYPE_LIGHT)

			Matrix         world;
			Color          color;
			OffsetT<int>   type;
			OffsetT<float> range;
			OffsetT<float> radius;
		};

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
	
	constexpr float __placeholder = 0.f;

	class ENGINE_SHADOWMANAGER_API ShadowManager : public Abstracts::Singleton<ShadowManager>
	{
	private:
		struct Subfrusta
		{
			Vector4 corners[8];
		};

	public:
		INLINE_COMPILE_TIME_TYPENAME(ShadowManager)
		explicit ShadowManager(SINGLETON_LOCK_TOKEN)
			: Singleton<ShadowManager>(),
			  m_viewport_() {}

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
		void RegisterLight(const Weak<Objects::Light>& light);
		void UnregisterLight(const Weak<Objects::Light>& light);

		static void EvalShadowVP(const Weak<Objects::Camera>& ptr_cam, const Vector3& light_dir, SBs::LightVPSB& buffer);
		void BindShadowMaps(const GraphicInterfaceContextPrimitive* context) const;
		void UnbindShadowMaps(const GraphicInterfaceContextPrimitive* context) const;

	private:
		friend struct SingletonDeleter;
		~ShadowManager() override;

		void InitializeViewport();
		void InitializeShadowBuffer(LocalActorID id);

		void BuildShadowMap(float dt, const Strong<Objects::Light>& light, UINT light_idx);
		void ClearShadowMaps(const GraphicInterfaceContextPrimitive* context);

		static void CreateSubfrusta(
			const Matrix& projection, float start, float end,
			Subfrusta&    subfrusta
		);

		Strong<Resources::Shader> m_shadow_shader_;
		Strong<Resources::Texture2D> m_shadow_map_mask_;
		std::map<LocalActorID, Strong<Resources::ShadowTexture>> m_shadow_texs_;

		// sub part of the view frustum
		Subfrusta m_subfrusta_[3];

		// lights from current scene
		std::map<LocalActorID, Weak<Objects::Light>> m_lights_;

		StructuredBufferTypeProxy<SBs::LightSB> m_light_sb_;
		StructuredBufferTypeProxy<SBs::LightVPSB> m_light_vp_sb_;

		Viewport m_viewport_;
	};
} // namespace Engine::Manager::Graphics
