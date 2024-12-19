#pragma once

#include "Source/Runtime/Core/Allocator/Public/Allocator.h"
#include "Source/Runtime/Resources/Shader/Public/Shader.hpp"
#include "Source/Runtime/Resources/ShadowTexture/Public/ShadowTexture.h"
#include "Source/Runtime/Resources/Texture2D/Public/Texture2D.h"

#include "RenderTask.h"

namespace Engine::Graphics
{
	namespace SBs
	{
		struct SHADOWMANAGER_API LightSB
		{
			SB_T(SB_TYPE_LIGHT)

			Matrix         world;
			Color          color;
			OffsetT<int>   type;
			OffsetT<float> range;
			OffsetT<float> radius;
		};

		struct SHADOWMANAGER_API LightVPSB
		{
			SB_T(SB_TYPE_LIGHT_VP)

			Matrix  view[CFG_CASCADE_SHADOW_COUNT];
			Matrix  proj[CFG_CASCADE_SHADOW_COUNT];
			Vector4 end_clip_spaces[CFG_CASCADE_SHADOW_COUNT];
		};
	}
}

namespace Engine
{
	struct SHADOWMANAGER_API ShadowRenderPrerequisiteTask : RenderPassPrerequisiteTask
	{
		void SetShadowShader(const Strong<Resources::Shader>& shader);
		void UpdateLight(const std::vector<Graphics::SBs::LightSB>& sb);
		void UpdateLightVP(const std::vector<Graphics::SBs::LightVPSB>& sb);

	protected:
		std::vector<Graphics::SBs::LightVPSB> m_light_vps_;
		std::vector<Graphics::SBs::LightSB> m_lights_;
		Strong<Resources::Shader> m_shadow_shader_;

		bool IsLazy() const;
		void FlipLazy();

	private:
		bool m_lazy_ = false;
	};
}

namespace Engine::Managers
{
	constexpr float __placeholder = 0.f;

	class SHADOWMANAGER_API ShadowManager : public Abstracts::Singleton<ShadowManager>
	{
	private:
		struct Subfrusta
		{
			Vector4 corners[8];
		};

	public:
		explicit ShadowManager(SINGLETON_LOCK_TOKEN)
			: Singleton<ShadowManager>(),
			  m_viewport_() {}

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void GetLightVP(const Strong<Scene>& scene, std::vector<SBs::LightVPSB>& current_light_vp);
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostUpdate(const float& dt) override;

		void Reset();
		void RegisterLight(const Weak<Objects::Light>& light);
		void UnregisterLight(const Weak<Objects::Light>& light);

		template <typename T> requires (std::is_base_of_v<ShadowRenderPrerequisiteTask, T>)
		void SetShadowRenderPrerequisiteTask()
		{
			m_shadow_task_ = std::make_unique<T>();
		}

		template <typename T> requires (std::is_base_of_v<ViewportRenderPrerequisiteTask, T>)
		void SetViewportRenderPrerequisiteTask()
		{
			m_viewport_task_ = std::make_unique<T>();
		}

		static void EvalShadowVP(const Weak<Objects::Camera>& ptr_cam, const Vector3& light_dir, SBs::LightVPSB& buffer);

	private:
		friend struct SingletonDeleter;
		~ShadowManager() override;

		void InitializeViewport();
		void InitializeShadowBuffer(LocalActorID id);

		void BuildShadowMap(float dt, const Strong<Objects::Light>& light, UINT light_idx);
		void ClearShadowMaps();

		static void CreateSubfrusta(
			const Matrix& projection, float start, float end,
			Subfrusta&    subfrusta
		);

		Strong<Resources::Shader> m_shadow_shader_;
		Strong<Resources::ShadowTexture> m_shadow_map_mask_;
		std::map<LocalActorID, Strong<Resources::ShadowTexture>> m_shadow_texs_;

		// sub part of the view frustum
		Subfrusta m_subfrusta_[3];

		// lights from current scene
		std::map<LocalActorID, Weak<Objects::Light>> m_lights_;

		// The DX resources for each of the shadow map (texture, depth stencil view and shader resource view)
		std::unique_ptr<ShadowRenderPrerequisiteTask> m_shadow_task_;
		std::unique_ptr<ViewportRenderPrerequisiteTask> m_viewport_task_;

		Viewport m_viewport_;
	};
} // namespace Engine::Manager::Graphics
