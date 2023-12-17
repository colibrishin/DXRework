#pragma once
#include "egCommon.hpp"
#include "egStateController.hpp"

namespace Engine::Components
{
    class ObserverController : public Abstract::StateController<eObserverState>
    {
    public:
        explicit ObserverController(const WeakObject& owner);

        void Initialize() override;
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;

    protected:
        ObserverController();

        virtual void Mouse(const float& dt);
        virtual void Move(const float& dt);

        Vector3 m_offset_;

    private:
        SERIALIZER_ACCESS
    };
} // namespace Engine::Component

BOOST_CLASS_EXPORT_KEY(Engine::Components::ObserverController);

BOOST_CLASS_EXPORT_KEY(
                       Engine::Abstract::StateController<Engine::eObserverState>);
