#pragma once
#include <SpriteBatch.h>
#include <CommonStates.h>

#include <fmod_studio.hpp>
#include "egCommon.hpp"
#include "egDXCommon.h"
#include "egD3Device.hpp"
#include "GeometricPrimitive.h"
#include "egRenderPipeline.hpp"

namespace Engine::Manager::Graphics
{
	class ToolkitAPI final : public Abstract::Singleton<ToolkitAPI>
	{
	public:
		ToolkitAPI(SINGLETON_LOCK_TOKEN) : Singleton() {}
		~ToolkitAPI() override;
		void Initialize() override;

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void FixedUpdate(const float& dt) override;

		void BeginPrimitiveBatch();
		void EndPrimitiveBatch();

		SpriteBatch* GetSpriteBatch() const { return m_sprite_batch_.get(); }
		CommonStates* GetCommonStates() const { return m_states_.get(); }
		PrimitiveBatch<VertexPositionColor>* GetPrimitiveBatch() const { return m_primitive_batch_.get(); }

		void LoadSound(FMOD::Sound** sound, const std::string& path) const;
		void PlaySound(FMOD::Sound* sound, const FMOD_VECTOR& pos, const FMOD_VECTOR& vel, FMOD::Channel** channel) const;
		void StopSound(FMOD::Sound* sound, FMOD::Channel** channel) const;

		void Set3DListener(const FMOD_VECTOR& position, const FMOD_VECTOR& velocity, const FMOD_VECTOR& forward, const FMOD_VECTOR& up) const;

	private:
		void FrameBegin() const;
		void FrameEnd() const;

		std::unique_ptr<CommonStates> m_states_ = nullptr;
		std::unique_ptr<GeometricPrimitive> m_geometric_primitive_ = nullptr;
		std::unique_ptr<SpriteBatch> m_sprite_batch_ = nullptr;

		std::unique_ptr<PrimitiveBatch<VertexPositionColor>> m_primitive_batch_ = nullptr;
		std::unique_ptr<BasicEffect> m_basic_effect_ = nullptr;
		ComPtr<ID3D11InputLayout> m_debug_input_layout_ = nullptr;

		FMOD::System* m_audio_engine_ = nullptr;
		FMOD::ChannelGroup* m_master_channel_group_ = nullptr;
		FMOD::ChannelControl* m_channel_control_ = nullptr;

	};
}
