#pragma once
#include "egCommon.hpp"

namespace Engine
{
    class Script : public Abstract::Renderable
    {
    public:
        explicit Script(const WeakObject& owner);

        void SetActive(const bool active);
        bool GetActive() const { return m_b_active_; }
        WeakObject GetOwner() const { return m_owner_; }

    protected:
        Script();

    private:
        SERIALIZER_ACCESS

        WeakObject m_owner_;
        bool m_b_active_;
    };
} // namespace Engine::Component

BOOST_CLASS_EXPORT_KEY(Engine::Script);
