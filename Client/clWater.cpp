#include "pch.h"
#include "clWater.hpp"

#include "egCubeMesh.hpp"
#include "egNormalMap.hpp"

SERIALIZER_ACCESS_IMPL(
                       Client::Object::Water,
                       _ARTAG(_BSTSUPER(Engine::Objects::DelayedRenderObject)))

void Client::Object::Water::Initialize()
{
    AddResource(
                Engine::GetResourceManager()
                .GetResource<Engine::Mesh::CubeMesh>("CubeMesh")
                .lock());
    AddResource(
                Engine::GetResourceManager()
                .GetResource<Engine::Resources::NormalMap>("WaterNormal")
                .lock());

    AddResource(
                Engine::GetResourceManager()
                .GetResource<Engine::Graphic::VertexShader>("vs_default")
                .lock());
    AddResource(
                Engine::GetResourceManager()
                .GetResource<Engine::Graphic::PixelShader>("ps_refraction")
                .lock());

    AddComponent<Engine::Component::Transform>();
    AddComponent<Engine::Component::Collider>();
}

void Client::Object::Water::PreUpdate(const float& dt)
{
    DelayedRenderObject::PreUpdate(dt);
}

void Client::Object::Water::Update(const float& dt)
{
    DelayedRenderObject::Update(dt);
}

void Client::Object::Water::PreRender(const float& dt)
{
    DelayedRenderObject::PreRender(dt);
}

void Client::Object::Water::Render(const float& dt)
{
    DelayedRenderObject::Render(dt);
}

void Client::Object::Water::PostRender(const float& dt)
{
    DelayedRenderObject::PostRender(dt);
}

void Client::Object::Water::FixedUpdate(const float& dt)
{
    DelayedRenderObject::FixedUpdate(dt);
}

void Client::Object::Water::OnDeserialized()
{
    DelayedRenderObject::OnDeserialized();
}

void Client::Object::Water::OnImGui()
{
    DelayedRenderObject::OnImGui();
}
