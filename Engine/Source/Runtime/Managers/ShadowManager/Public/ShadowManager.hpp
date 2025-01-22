#pragma once
#include <Source/Runtime/TypeLibrary/Public/TypeLibrary.h>
#include <Source/Runtime/StructuredBufferDX12/Public/StructuredBufferDX12.hpp>

namespace Engine::Graphics
{
	namespace SBs 
	{
		struct LightSB
		{
			SB_T(SB_TYPE_LIGHT)

			Matrix         world;
			Color          color;
			OffsetT<int>   type;
			OffsetT<float> range;
			OffsetT<float> radius;
		};

		struct LightVPSB
		{
			SB_T(SB_TYPE_LIGHT_VP)

			Matrix  view[CASCADE_SHADOW_COUNT];
			Matrix  proj[CASCADE_SHADOW_COUNT];
			Vector4 end_clip_spaces[CASCADE_SHADOW_COUNT];
		};
	}
}

namespace Engine::Managers
{
	constexpr float __placeholder = 0.f;

	class ShadowManager : public Abstracts::Singleton<ShadowManager>
	{
	private:
		struct Subfrusta
		{
			Vector4 corners[8];
		};

	public:
		explicit ShadowManager(SINGLETON_LOCK_TOKEN)
			: Singleton<ShadowManager>(),
			  m_viewport_(),
			  m_scissor_rect_(),
			  m_shadow_map_mask_("", {}) {}

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void GetLightVP(const boost::shared_ptr<Scene>& scene, std::vector<SBs::LightVPSB>& current_light_vp);
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostUpdate(const float& dt) override;

		void Reset();
		void RegisterLight(const Weak<Objects::Light>& light);
		void UnregisterLight(const Weak<Objects::Light>& light);

		static void EvalShadowVP(const Weak<Objects::Camera>& ptr_cam, const Vector3& light_dir, SBs::LightVPSB& buffer);

		void BindShadowMaps(const Weak<CommandPair>& cmd, const DescriptorPtr& heap) const;
		void BindShadowSampler(const DescriptorPtr& heap) const;
		void UnbindShadowMaps(const Weak<CommandPair>& w_cmd) const;

		Graphics::StructuredBuffer<SBs::LightSB>*   GetLightBuffer();
		Graphics::StructuredBuffer<SBs::LightVPSB>* GetLightVPBuffer();

	private:
		friend struct SingletonDeleter;
		~ShadowManager() override;

		void InitializeViewport();
		void InitializeProcessor();
		void InitializeShadowBuffer(LocalActorID id);

		void BuildShadowMap(
			float dt, const Weak<CommandPair>& w_cmd, const Strong<Objects::Light>& light, UINT
			light_idx
		);
		void ClearShadowMaps(const Weak<CommandPair>& w_cmd);

		static void CreateSubfrusta(
			const Matrix& projection, float start, float end,
			Subfrusta&    subfrusta
		);

		Strong<Resources::Shader> m_shadow_shader_;

		// sub part of the view frustum
		Subfrusta m_subfrusta_[3];

		// lights from current scene
		std::map<LocalActorID, Weak<Objects::Light>> m_lights_;

		// The DX resources for each of the shadow map (texture, depth stencil view and shader resource view)
		std::map<LocalActorID, Resources::ShadowTexture> m_shadow_texs_;

		// light structured buffer
		Graphics::StructuredBuffer<SBs::LightSB> m_sb_light_buffer_{};
		// The structured buffer for the chunk of light view projection matrices
		Graphics::StructuredBuffer<SBs::LightVPSB> m_sb_light_vps_buffer_;


		D3D12_VIEWPORT m_viewport_;
		D3D12_RECT     m_scissor_rect_;

		DescriptorContainer m_shadow_descriptor_heap_;

		StructuredBufferMemoryPool<SBs::InstanceSB>   m_shadow_instance_buffer_;
		StructuredBufferMemoryPool<SBs::LocalParamSB> m_local_param_buffers_;

		Resources::Texture2D m_shadow_map_mask_;

		ComPtr<ID3D12DescriptorHeap> m_sampler_heap_;
	};
} // namespace Engine::Manager::Graphics
