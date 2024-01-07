#pragma once
#include "pch.h"

namespace Engine::Manager::Graphics
{
    class ReflectionEvaluator : public Abstract::Singleton<ReflectionEvaluator>
    {
    public:
        ReflectionEvaluator(SINGLETON_LOCK_TOKEN) : Singleton() {}
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;
        void PostUpdate(const float& dt) override;
        void Initialize() override;

        void RenderFinished();

    private:
        friend struct SingletonDeleter;
        ~ReflectionEvaluator() override = default;

        Graphics::DXPacked::RenderedResource m_rendered_buffer_;

    };
}