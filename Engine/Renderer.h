#pragma once
#include "egManager.hpp"
#include "egModelRenderer.h"

namespace Engine::Manager::Graphics
{
	class Renderer : public Abstract::Singleton<Renderer>
    {
    public:
        Renderer(SINGLETON_LOCK_TOKEN) : Singleton() {}
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;
        void PostUpdate(const float& dt) override;
        void Initialize() override;

	private:
        void RenderModel(const float& dt, const WeakModelRenderer& ptr_mr, const WeakTransform& ptr_tr, const WeakAnimator& ptr_atr);

        std::queue<WeakObject> m_delayed_objects_;
    };
}


