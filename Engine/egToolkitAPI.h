#pragma once
#include <CommonStates.h>
#include <SpriteBatch.h>

#include <fmod_studio.hpp>
#include "GeometricPrimitive.h"
#include "egD3Device.hpp"

namespace Engine::Manager::Graphics
{
    class ToolkitAPI final : public Abstract::Singleton<ToolkitAPI>
    {
    public:
        ToolkitAPI(SINGLETON_LOCK_TOKEN) {}

        void Initialize() override;
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PostUpdate(const float& dt) override;

        void BeginPrimitiveBatch();
        void EndPrimitiveBatch();

        SpriteBatch* GetSpriteBatch() const;
        CommonStates* GetCommonStates() const;
        PrimitiveBatch<VertexPositionColor>* GetPrimitiveBatch() const;

        void LoadSound(FMOD::Sound** sound, const std::string& path) const;
        void PlaySound(
            FMOD::Sound*       sound, const FMOD_VECTOR& pos,
            const FMOD_VECTOR& vel, FMOD::Channel**      channel) const;
        void StopSound(FMOD::Sound* sound, FMOD::Channel** channel) const;

        void Set3DListener(
            const FMOD_VECTOR& position, const FMOD_VECTOR& velocity,
            const FMOD_VECTOR& forward, const FMOD_VECTOR&  up) const;

    private:
        friend struct SingletonDeleter;
        ~ToolkitAPI() override;

        void FrameBegin() const;
        void FrameEnd() const;

        std::unique_ptr<CommonStates>       m_states_              = nullptr;
        std::unique_ptr<GeometricPrimitive> m_geometric_primitive_ = nullptr;
        std::unique_ptr<SpriteBatch>        m_sprite_batch_        = nullptr;

        std::unique_ptr<PrimitiveBatch<VertexPositionColor>> m_primitive_batch_ =
                nullptr;
        std::unique_ptr<BasicEffect> m_basic_effect_       = nullptr;
        ComPtr<ID3D11InputLayout>    m_debug_input_layout_ = nullptr;

        FMOD::System*         m_audio_engine_         = nullptr;
        FMOD::ChannelGroup*   m_master_channel_group_ = nullptr;
        FMOD::ChannelControl* m_channel_control_      = nullptr;
    };
} // namespace Engine::Manager::Graphics
