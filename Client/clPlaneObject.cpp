#include "pch.h"
#include "clPlaneObject.hpp"

#include <egCubeMesh.h>
#include <egObject.hpp>
#include <egSound.h>
#include <egVertexShaderInternal.h>

#include "egBaseCollider.hpp"
#include "egModelRenderer.h"
#include "egShader.hpp"
#include "egTransform.h"
#include "egMaterial.h"
#include "egRigidbody.h"
#include "egShape.h"

SERIALIZER_ACCESS_IMPL(
                       Client::Object::PlaneObject,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Object)))

namespace Client::Object
{
    PlaneObject::PlaneObject() {}

    void PlaneObject::Initialize()
    {
        const auto model = Resources::Shape::Get("CubeModel").lock();

        const auto mr = AddComponent<Components::ModelRenderer>().lock();
        mr->SetMaterial(Resources::Material::Get("ColorMaterial"));
        mr->SetShape(model);

        AddComponent<Components::Transform>();
        const auto tr = GetComponent<Components::Transform>().lock();
        tr->SetLocalPosition(Vector3(0.0f, -1.0f, 0.0f));
        tr->SetLocalScale({10.0f, 1.0f, 10.0f});

        AddComponent<Components::BaseCollider>();
        const auto cldr = GetComponent<Components::BaseCollider>().lock();
        cldr->SetType(Engine::BOUNDING_TYPE_BOX);
        cldr->SetMass(100000.0f);

        AddComponent<Components::Rigidbody>();
        const auto rb = GetComponent<Components::Rigidbody>().lock();

        rb->SetFixed(true);
        rb->SetFrictionCoefficient(0.2f);
        rb->SetGravityOverride(false);
    }

    PlaneObject::~PlaneObject() {}

    void PlaneObject::PreUpdate(const float& dt)
    {
        Object::PreUpdate(dt);
    }

    void PlaneObject::Update(const float& dt)
    {
        Object::Update(dt);
    }

    void PlaneObject::PreRender(const float& dt)
    {
        Object::PreRender(dt);
    }

    void PlaneObject::Render(const float& dt)
    {
        Object::Render(dt);
    }

    void PlaneObject::PostRender(const float& dt)
    {
        Object::PostRender(dt);
    }
} // namespace Client::Object
