#include "pch.h"
#include "clWater.hpp"

#include <egCubeMesh.h>
#include <egNormalMap.h>

#include "egModel.h"
#include "egModelRenderer.h"
#include "egShader.hpp"

SERIALIZER_ACCESS_IMPL(
                       Client::Object::Water,
                       _ARTAG(_BSTSUPER(Engine::Objects::DelayedRenderObject)))

void Client::Object::Water::Initialize()
{
    const auto mr = AddComponent<Engine::Components::ModelRenderer>().lock();
    const auto model = Engine::Resources::Model::Get("CubeModel");

    mr->SetModel(model);
    mr->AddVertexShader(Engine::Graphic::VertexShader::Get("vs_default"));
    mr->AddPixelShader(Engine::Graphic::PixelShader::Get("ps_refraction"));

    AddComponent<Engine::Components::Transform>();
    const auto cldr = AddComponent<Engine::Components::Collider>().lock();

    cldr->SetBoundingBox(model.lock()->GetBoundingBox());
    cldr->SetOffsetPosition({0.f, 0.5f, 0.f});

    const auto cldr2 = AddComponent<Engine::Components::Collider>().lock();
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
