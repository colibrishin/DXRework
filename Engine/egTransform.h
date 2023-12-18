#pragma once
#include <SimpleMath.h>
#include <boost/serialization/export.hpp>
#include "egComponent.h"
#include "egDXCommon.h"

namespace Engine::Components
{
    class Transform : public Abstract::Component
    {
    public:
        Transform(const WeakObject& owner);
        ~Transform() override = default;

        void SetPosition(const Vector3& position);
        void SetRotation(const Quaternion& rotation);
        void SetYawPitchRoll(const Vector3& yaw_pitch_roll);
        void SetScale(const Vector3& scale);
        void Translate(Vector3 translation);

        Vector3    GetPosition() const;
        Vector3    GetPreviousPosition() const;
        Quaternion GetRotation() const;
        Vector3    GetScale() const;

        void Initialize() override;
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void PostRender(const float& dt) override;
        void Render(const float& dt) override;
        void FixedUpdate(const float& dt) override;

        void     OnImGui() override;
        void     OnDeserialized() override;

        Matrix GetWorldMatrix() const;

    protected:
        Transform();

    private:
        friend class Manager::Physics::LerpManager;
        friend class Manager::Graphics::ShadowManager;

        SERIALIZER_ACCESS

        Vector3    m_previous_position_;
        Vector3    m_position_;
        Vector3    m_yaw_pitch_roll_degree_;
        Quaternion m_rotation_;
        Vector3    m_scale_;

        // Non-serialized
        TransformBuffer m_transform_buffer_;
    };
} // namespace Engine::Component

BOOST_CLASS_EXPORT_KEY(Engine::Components::Transform)
