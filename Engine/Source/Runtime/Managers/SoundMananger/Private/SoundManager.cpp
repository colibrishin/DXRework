#include "../Public/SoundManager.h"

#include "Components/Rigidbody/Public/Rigidbody.h"
#include "Components/Transform/Public/Transform.h"

#include "Objects/Camera/Public/Camera.h"

#include "Scene/Public/Scene.hpp"

#include "SceneManager/Public/SceneManager.hpp"

#pragma comment(lib, "fmod_vc.lib")

namespace FMOD 
{
	void ThrowIfFailed(FMOD_RESULT hr);
	const char* fmod_exception::what() const noexcept
	{

		static char s_str[64] = {};
		sprintf_s
		(
			s_str, "Failure with FMOD_RESULT of %08X",
			static_cast<unsigned int>(result)
		);
		return s_str;
	}

	void ThrowIfFailed(FMOD_RESULT hr)
	{
		if (hr != FMOD_OK)
		{
			throw fmod_exception(hr);
		}
	}
}

namespace Engine::Managers
{
	SoundManager::~SoundManager()
	{
		m_audio_engine_->update();
		m_audio_engine_->release();
	}

	void SoundManager::Initialize()
	{
		FMOD::ThrowIfFailed(System_Create(&m_audio_engine_));

		FMOD::ThrowIfFailed(m_audio_engine_->init(32, FMOD_INIT_NORMAL, nullptr));

		FMOD::ThrowIfFailed
		(
			m_audio_engine_->createChannelGroup("Master", &m_master_channel_group_)
		);

		m_master_channel_group_->setVolume(0.5f);
	}

	void SoundManager::PreUpdate(const float dt) { }

	void SoundManager::Update(const float dt) {}

	void SoundManager::PreRender(const float dt) {}

	void SoundManager::Render(const float dt) {}

	void SoundManager::PostRender(const float dt) {}

	void SoundManager::FixedUpdate(const float dt) { }

	void SoundManager::PostUpdate(const float dt)
	{
		if (const Strong<Scene>& scene = SceneManager::GetInstance().GetActiveScene().lock())
		{
			if (const Strong<Objects::Camera>& camera = scene->GetMainCamera().lock())
			{
				Vector3 velocity = Vector3::Zero;

				if (const auto bound = camera->GetParent().lock())
				{
					if (const auto rb = bound->GetComponent<Components::Rigidbody>().lock())
					{
						velocity = rb->GetT0LinearVelocity();
					}
				}

				const Matrix invView = camera->GetViewMatrix().Invert();

				Set3DListener
						(
						{invView._41, invView._42, invView._43},
						{velocity.x, velocity.y, velocity.z},
						{g_forward.x, g_forward.y, g_forward.z},
						{Vector3::Up.x, Vector3::Up.y, Vector3::Up.z}
						); // todo: fixed up?
			}
		}

		m_audio_engine_->update();
	}

	void SoundManager::LoadSound(FMOD::Sound** sound, const std::string& path) const
	{
		FMOD::ThrowIfFailed
		(
			m_audio_engine_->createSound
			(
				path.c_str(), FMOD_3D | FMOD_3D_LINEARROLLOFF, nullptr,
				sound
			)
		);
	}

	void SoundManager::PlaySound(
		FMOD::Sound* sound, const FMOD_VECTOR& pos,
		const FMOD_VECTOR& vel,
		FMOD::Channel** channel
	) const
	{
		FMOD::ThrowIfFailed
		(
			m_audio_engine_->playSound
			(
				sound, m_master_channel_group_, false, channel
			)
		);

		if (pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f && vel.x == 0.0f &&
			vel.y == 0.f && vel.z == 0.f)
		{
			return;
		}

		FMOD::ThrowIfFailed((*channel)->set3DAttributes(&pos, &vel));
		FMOD::ThrowIfFailed((*channel)->set3DSpread(360.0f));
	}

	void SoundManager::StopSound(FMOD::Sound* sound, FMOD::Channel** channel) const
	{
		FMOD::ThrowIfFailed
		(
			m_audio_engine_->playSound
			(
				sound, m_master_channel_group_, true, channel
			)
		);
		FMOD::ThrowIfFailed((*channel)->set3DAttributes(nullptr, nullptr));
	}

	void SoundManager::Set3DListener(
		const FMOD_VECTOR& position,
		const FMOD_VECTOR& velocity,
		const FMOD_VECTOR& forward,
		const FMOD_VECTOR& up
	) const
	{
		FMOD::ThrowIfFailed
		(
			m_audio_engine_->set3DListenerAttributes
			(
				0, &position, &velocity, &forward, &up
			)
		);
	}
} // namespace Engine::Manager::Graphics
