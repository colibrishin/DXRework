#include "pch.h"
#include "egDelayedRenderObject.h"

#include "egCubeMesh.h"
#include "egNormalMap.h"
#include "egVertexShaderInternal.h"

#include "egCollider.hpp"
#include "egTransform.h"

SERIALIZER_ACCESS_IMPL(
                       Engine::Objects::DelayedRenderObject,
                       _ARTAG(_BSTSUPER(Object)))

Engine::Objects::DelayedRenderObject::DelayedRenderObject(): Abstract::Object(DEF_OBJ_T_DELAY_OBJ) {}

void Engine::Objects::DelayedRenderObject::Initialize()
{
    Object::Initialize();
}

void Engine::Objects::DelayedRenderObject::PreUpdate(const float& dt)
{
    Object::PreUpdate(dt);
}

void Engine::Objects::DelayedRenderObject::Update(const float& dt)
{
    Object::Update(dt);
}

void Engine::Objects::DelayedRenderObject::PreRender(const float& dt)
{
    Object::PreRender(dt);
}

void Engine::Objects::DelayedRenderObject::Render(const float& dt) {}

void Engine::Objects::DelayedRenderObject::PostRender(const float& dt)
{
    Object::Render(dt);
    Object::PostRender(dt);
}

void Engine::Objects::DelayedRenderObject::FixedUpdate(const float& dt)
{
    Object::FixedUpdate(dt);
}

void Engine::Objects::DelayedRenderObject::OnDeserialized()
{
    Object::OnDeserialized();
}

void Engine::Objects::DelayedRenderObject::OnImGui()
{
    Object::OnImGui();
}
