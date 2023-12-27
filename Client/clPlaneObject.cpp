#include "pch.h"
#include "clPlaneObject.hpp"

#include <egCubeMesh.h>
#include <egObject.hpp>
#include <egSound.h>
#include <egVertexShaderInternal.h>
#include <egNormalMap.h>

#include "egModel.h"
#include "egModelRenderer.h"
#include "egShader.hpp"
#include "egTransform.h"
#include "egCollider.hpp"
#include "egRigidbody.h"

SERIALIZER_ACCESS_IMPL(
                       Client::Object::PlaneObject,
                       _ARTAG(_BSTSUPER(Engine::Abstract::Object)))

namespace Client::Object
{
    PlaneObject::PlaneObject() {}

    void PlaneObject::Initialize()
    {
        const auto model = Resources::Model::Get("CubeModel").lock();

        const auto mr = AddComponent<Components::ModelRenderer>().lock();
        mr->SetModel(model);
        mr->AddVertexShader(Resources::VertexShader::Get("vs_default").lock());
        mr->AddPixelShader(Resources::PixelShader::Get("ps_color").lock());

        AddComponent<Components::Transform>();
        const auto tr = GetComponent<Components::Transform>().lock();
        tr->SetLocalPosition(Vector3(0.0f, -1.0f, 0.0f));
        tr->SetScale({10.0f, 1.0f, 10.0f});

        AddComponent<Components::Collider>();
        const auto cldr = GetComponent<Components::Collider>().lock();
        cldr->SetBoundingBox(model->GetBoundingBox());
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
