#include "pch.hpp"
#include "egToolkitAPI.hpp"
#include "egSceneManager.hpp"
#include "egCamera.hpp"

namespace Engine::Manager::Graphics 
{
	ToolkitAPI::~ToolkitAPI()
	{
		m_audio_engine_->update();
		m_audio_engine_->release();
	}

	void ToolkitAPI::Initialize()
	{
		m_states_ = std::make_unique<CommonStates>(GetD3Device().GetDevice());
		m_geometric_primitive_ = GeometricPrimitive::CreateTeapot(GetD3Device().GetContext());
		GeometricPrimitive::SetDepthBufferMode(false);

		m_sprite_batch_ = std::make_unique<SpriteBatch>(GetD3Device().GetContext());
		m_primitive_batch_ = std::make_unique<PrimitiveBatch<VertexPositionColor>>(GetD3Device().GetContext());
		m_basic_effect_ = std::make_unique<BasicEffect>(GetD3Device().GetDevice());
		m_basic_effect_->SetVertexColorEnabled(true);

		void const* shaderByteCode;
		size_t byteCodeLength;

		m_basic_effect_->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);
		DX::ThrowIfFailed(GetD3Device().GetDevice()->CreateInputLayout(VertexPositionColor::InputElements, VertexPositionColor::InputElementCount,
            shaderByteCode, byteCodeLength,
            m_debug_input_layout_.ReleaseAndGetAddressOf()));


		FMOD::DX::ThrowIfFailed(FMOD::System_Create(&m_audio_engine_));

		FMOD::DX::ThrowIfFailed(m_audio_engine_->init(32, FMOD_INIT_NORMAL, nullptr));

		FMOD::DX::ThrowIfFailed(m_audio_engine_->createChannelGroup("Master", &m_master_channel_group_));

		m_master_channel_group_->setVolume(0.5f);
	}

	void ToolkitAPI::PreUpdate(const float& dt)
	{
	}

	void ToolkitAPI::Update(const float& dt)
	{
		m_audio_engine_->update();
	}

	void ToolkitAPI::PreRender(const float& dt)
	{
		FrameBegin();
	}

	void ToolkitAPI::Render(const float& dt)
	{
		FrameEnd();
	}

	void ToolkitAPI::FixedUpdate(const float& dt)
	{
	}

	void ToolkitAPI::BeginPrimitiveBatch()
	{
		const auto context = GetD3Device().GetContext();

		context->OMSetBlendState(m_states_->Opaque(), nullptr, 0xFFFFFFFF);
		context->OMSetDepthStencilState(m_states_->DepthNone(), 0);
		context->RSSetState(m_states_->CullNone());

		m_basic_effect_->Apply(context);
		context->IASetInputLayout(m_debug_input_layout_.Get());

		if (const auto scene = GetSceneManager().GetActiveScene().lock())
		{
			if (const auto camera = scene->GetMainCamera().lock())
			{
				m_basic_effect_->SetView(camera->GetViewMatrix());
				m_basic_effect_->SetProjection(camera->GetProjectionMatrix());
			}
		}

		m_primitive_batch_->Begin();
	}

	void ToolkitAPI::EndPrimitiveBatch()
	{
		GetD3Device().GetContext()->RSSetState(GetRenderPipeline().m_rasterizer_state_.Get());
		GetD3Device().GetContext()->OMSetBlendState(GetRenderPipeline().m_blend_state_.Get(), nullptr, 0xFFFFFFFF);
		GetD3Device().GetContext()->OMSetDepthStencilState(GetRenderPipeline().m_depth_stencil_state_.Get(), 1);

		m_primitive_batch_->End();
	}

	void ToolkitAPI::LoadSound(FMOD::Sound** sound, const std::string& path) const
	{
		FMOD::DX::ThrowIfFailed(m_audio_engine_->createSound(path.c_str(), FMOD_3D | FMOD_3D_LINEARROLLOFF, nullptr, sound));
	}

	void ToolkitAPI::PlaySound(FMOD::Sound* sound, const FMOD_VECTOR& pos, const FMOD_VECTOR& vel, FMOD::Channel** channel) const
	{
		FMOD::DX::ThrowIfFailed(m_audio_engine_->playSound(sound, m_master_channel_group_, false, channel));

		if (pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f && vel.x == 0.0f && vel.y == 0.f && vel.z == 0.f)
		{
			return;
		}

		FMOD::DX::ThrowIfFailed((*channel)->set3DAttributes(&pos, &vel));
		FMOD::DX::ThrowIfFailed((*channel)->set3DSpread(360.0f));
	}

	void ToolkitAPI::StopSound(FMOD::Sound* sound, FMOD::Channel** channel) const
	{
		FMOD::DX::ThrowIfFailed(m_audio_engine_->playSound(sound, m_master_channel_group_, true, channel));
		FMOD::DX::ThrowIfFailed((*channel)->set3DAttributes(nullptr, nullptr));
	}

	void ToolkitAPI::Set3DListener(const FMOD_VECTOR& position, const FMOD_VECTOR& velocity, const FMOD_VECTOR& forward,
		const FMOD_VECTOR& up) const
	{
		FMOD::DX::ThrowIfFailed(m_audio_engine_->set3DListenerAttributes(0, &position, &velocity, &forward, &up));
	}

	void ToolkitAPI::FrameBegin() const
	{
		m_sprite_batch_->Begin(
			SpriteSortMode_Deferred, 
			GetRenderPipeline().m_blend_state_.Get(), 
			GetRenderPipeline().s_sampler_state_[eShaderType::SHADER_PIXEL],
			GetRenderPipeline().m_depth_stencil_state_.Get(),
			GetRenderPipeline().m_rasterizer_state_.Get());
	}

	void ToolkitAPI::FrameEnd() const
	{
		m_sprite_batch_->End();
	}
}
