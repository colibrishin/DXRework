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

		SpriteBatch* GetSpriteBatch() const { return m_sprite_batch_.get(); }
		CommonStates* GetCommonStates() const { return m_states_.get(); }

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

		FMOD::System* m_audio_engine_ = nullptr;
		FMOD::ChannelGroup* m_master_channel_group_ = nullptr;
		FMOD::ChannelControl* m_channel_control_ = nullptr;

	};

	inline ToolkitAPI::~ToolkitAPI()
	{
		m_audio_engine_->update();
		m_audio_engine_->release();
	}

	inline void ToolkitAPI::Initialize()
	{
		m_states_ = std::make_unique<CommonStates>(GetD3Device().GetDevice());
		m_geometric_primitive_ = GeometricPrimitive::CreateTeapot(GetD3Device().GetContext());
		GeometricPrimitive::SetDepthBufferMode(true);

		m_sprite_batch_ = std::make_unique<SpriteBatch>(GetD3Device().GetContext());

		FMOD::DX::ThrowIfFailed(FMOD::System_Create(&m_audio_engine_));

		FMOD::DX::ThrowIfFailed(m_audio_engine_->init(32, FMOD_INIT_NORMAL, nullptr));

		FMOD::DX::ThrowIfFailed(m_audio_engine_->createChannelGroup("Master", &m_master_channel_group_));

		m_master_channel_group_->setVolume(0.5f);
	}

	inline void ToolkitAPI::PreUpdate(const float& dt)
	{
	}

	inline void ToolkitAPI::Update(const float& dt)
	{
		m_audio_engine_->update();
	}

	inline void ToolkitAPI::PreRender(const float& dt)
	{
		FrameBegin();
	}

	inline void ToolkitAPI::Render(const float& dt)
	{
		FrameEnd();
	}

	inline void ToolkitAPI::FixedUpdate(const float& dt)
	{
	}

	inline void ToolkitAPI::LoadSound(FMOD::Sound** sound, const std::string& path) const
	{
		FMOD::DX::ThrowIfFailed(m_audio_engine_->createSound(path.c_str(), FMOD_3D | FMOD_3D_LINEARROLLOFF, nullptr, sound));
	}

	inline void ToolkitAPI::PlaySound(FMOD::Sound* sound, const FMOD_VECTOR& pos, const FMOD_VECTOR& vel, FMOD::Channel** channel) const
	{
		FMOD::DX::ThrowIfFailed(m_audio_engine_->playSound(sound, m_master_channel_group_, false, channel));

		if (pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f && vel.x == 0.0f && vel.y == 0.f && vel.z == 0.f)
		{
			return;
		}

		FMOD::DX::ThrowIfFailed((*channel)->set3DAttributes(&pos, &vel));
		FMOD::DX::ThrowIfFailed((*channel)->set3DSpread(360.0f));
	}

	inline void ToolkitAPI::StopSound(FMOD::Sound* sound, FMOD::Channel** channel) const
	{
		FMOD::DX::ThrowIfFailed(m_audio_engine_->playSound(sound, m_master_channel_group_, true, channel));
		FMOD::DX::ThrowIfFailed((*channel)->set3DAttributes(nullptr, nullptr));
	}

	inline void ToolkitAPI::Set3DListener(const FMOD_VECTOR& position, const FMOD_VECTOR& velocity, const FMOD_VECTOR& forward,
		const FMOD_VECTOR& up) const
	{
		FMOD::DX::ThrowIfFailed(m_audio_engine_->set3DListenerAttributes(0, &position, &velocity, &forward, &up));
	}

	inline void ToolkitAPI::FrameBegin() const
	{
		m_sprite_batch_->Begin(
			SpriteSortMode_Deferred, 
			GetRenderPipeline().m_blend_state_.Get(), 
			GetRenderPipeline().s_sampler_state_[eShaderType::SHADER_PIXEL],
			GetRenderPipeline().m_depth_stencil_state_.Get(),
			GetRenderPipeline().m_rasterizer_state_.Get());
	}

	inline void ToolkitAPI::FrameEnd() const
	{
		m_sprite_batch_->End();
	}
}
