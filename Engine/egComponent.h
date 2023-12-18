#pragma once
#include "egCommon.hpp"
#include "egRenderable.h"

namespace Engine::Abstract
{
    class Component : public Renderable
    {
    public:
        ~Component() override       = default;
        Component(const Component&) = default;

        WeakObject         GetOwner() const;
        eComponentType GetComponentType() const;
        ComponentID        GetLocalID() const;
        bool               IsTicked() const;
        bool               GetActive() const;

        void SetActive(bool active);

        void OnImGui() override;
        void Render(const float& dt) override;

    protected:
        Component(eComponentType type, const WeakObject& owner);

    private:
        SERIALIZER_ACCESS
        friend class Object;

        void SetOwner(const WeakObject& owner)
        {
            m_owner_ = owner;
        }

        void SetLocalID(ComponentID id)
        {
            if (const auto locked = m_owner_.lock())
            {
                m_local_id_ = id;
            }
        }

    private:
        ComponentID        m_local_id_;
        eComponentType m_type_;

        // Non-serialized
        WeakObject m_owner_;
        bool       m_b_ticked_;
        bool       m_b_active_;
    };
} // namespace Engine::Abstract

BOOST_CLASS_EXPORT_KEY(Engine::Abstract::Component)
