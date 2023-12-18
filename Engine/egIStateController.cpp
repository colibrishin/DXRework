#include "pch.h"
#include "egIStateController.h"

SERIALIZER_ACCESS_IMPL(
                       Engine::Abstract::IStateController,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Component)))

Engine::Abstract::IStateController::IStateController(const Component& component)
: Component(component) {}

Engine::Abstract::IStateController::IStateController(eComponentType priority, const WeakObject& owner)
: Component(priority, owner) {}
