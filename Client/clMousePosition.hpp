#pragma once

#include "clTriangleMesh.hpp"
#include "../Engine/egCollider.hpp"
#include "../Engine/egIShader.hpp"
#include "../Engine/egManagerHelper.hpp"
#include "../Engine/egObject.hpp"
#include "../Engine/egResourceManager.hpp"
#include "../Engine/egRigidbody.hpp"
#include "../Engine/egText.hpp"
#include "../Engine/egTexture.hpp"
#include "../Engine/egTransform.hpp"

namespace Engine::Component
{
    class Rigidbody;
    class Transform;
} // namespace Engine::Component

namespace Client::Object
{
    class MousePositionText : public Engine::Objects::Text
    {
    public:
        MousePositionText();
        void Initialize() override;
        ~MousePositionText() override;

        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;

    private:
        SERIALIZER_ACCESS
    };
} // namespace Client::Object

BOOST_CLASS_EXPORT_KEY(Client::Object::MousePositionText);
