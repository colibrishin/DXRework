#pragma once

#include "clTriangleMesh.hpp"
#include "egSound.hpp"
#include "../Engine/egCollider.hpp"
#include "../Engine/egDebugger.hpp"
#include "../Engine/egIShader.hpp"
#include "../Engine/egManagerHelper.hpp"
#include "../Engine/egNormalMap.hpp"
#include "../Engine/egObject.hpp"
#include "../Engine/egResourceManager.hpp"
#include "../Engine/egRigidbody.hpp"
#include "../Engine/egTexture.hpp"
#include "../Engine/egTransform.hpp"

namespace Client::Object
{
    class PlaneObject : public Engine::Abstract::Object
    {
    public:
        PlaneObject();
        void Initialize() override;
        ~PlaneObject() override;

        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;

    private:
        SERIALIZER_ACCESS
    };
} // namespace Client::Object

BOOST_CLASS_EXPORT_KEY(Client::Object::PlaneObject);
