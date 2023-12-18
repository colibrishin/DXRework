#pragma once
#include "egComponent.h"

namespace Engine::Abstract
{
    // Empty interface for distinguishing state controllers
    class IStateController : public Component
    {
    public:
        virtual  ~IStateController() = default;

        explicit IStateController(const Component& component);

        IStateController(eComponentType priority, const WeakObject& owner);

    private:
        SERIALIZER_ACCESS
    };
} // namespace Engine::Abstract

BOOST_CLASS_EXPORT_KEY(Engine::Abstract::IStateController)
