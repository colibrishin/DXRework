#include "pch.h"
#include "clWater.hpp"

#include <egCubeMesh.h>

#include "egBaseCollider.hpp"
#include "egMaterial.h"
#include "egModelRenderer.h"
#include "egShader.hpp"
#include "egShape.h"
#include "egTransform.h"

SERIALIZER_ACCESS_IMPL
(
 Client::Object::Water,
 _ARTAG(_BSTSUPER(Engine::Abstract::Object))
)

void Client::Object::Water::Initialize()
{
  SetName("Water");
  const auto mr    = AddComponent<Components::ModelRenderer>().lock();
  const auto model = Resources::Shape::Get("CubeShape");

  mr->SetMaterial(Resources::Material::Get("WaterCube"));

  AddComponent<Components::Transform>();
  const auto cldr = AddComponent<Components::Collider>().lock();
  cldr->SetModel(model);
}

void Client::Object::Water::PreUpdate(const float& dt) { Object::PreUpdate(dt); }

void Client::Object::Water::Update(const float& dt) { Object::Update(dt); }

void Client::Object::Water::PreRender(const float& dt) { Object::PreRender(dt); }

void Client::Object::Water::Render(const float& dt) { Object::Render(dt); }

void Client::Object::Water::PostRender(const float& dt) { Object::PostRender(dt); }

void Client::Object::Water::FixedUpdate(const float& dt) { Object::FixedUpdate(dt); }

void Client::Object::Water::OnDeserialized() { Object::OnDeserialized(); }

void Client::Object::Water::OnImGui() { Object::OnImGui(); }
