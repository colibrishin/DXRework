#pragma once
#include <SimpleMath.h>
#include <boost/serialization/export.hpp>
#include "egComponent.h"
#include "egDXCommon.h"

namespace Engine::Components
{
    class Transform final : public Abstract::Component
    {
    public:
        INTERNAL_COMP_CHECK_CONSTEXPR(COM_T_TRANSFORM)

        Transform(const WeakObject& owner);
        ~Transform() override = default;

        void SetWorldPosition(const Vector3& position);
        void SetWorldRotation(const Quaternion& rotation);

        void SetLocalPosition(const Vector3& position);
        void SetLocalRotation(const Quaternion& rotation);
        void SetScale(const Vector3& scale);
        void SetSizeAbsolute(bool absolute);

        void SetAnimationPosition(const Vector3& position);
        void SetAnimationRotation(const Quaternion& rotation);
        void SetAnimationScale(const Vector3& scale);

        Vector3 GetWorldPosition() const;
        Quaternion GetWorldRotation() const;

        Vector3    GetWorldPreviousPosition() const;
        Vector3    GetLocalPreviousPosition() const;
        Vector3    GetLocalPosition() const;
        Quaternion GetLocalRotation() const;
        Vector3    GetScale() const;

        Vector3 Forward() const;
        Vector3 Right() const;
        Vector3 Up() const;

        void SetYawPitchRoll(const Vector3& yaw_pitch_roll);
        void Translate(Vector3 translation);

        void Initialize() override;
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PostUpdate(const float& dt) override;
        void FixedUpdate(const float& dt) override;

        void     OnImGui() override;
        void     OnDeserialized() override;

        Matrix GetLocalMatrix() const;
        Matrix     GetWorldMatrix() const;

        static void Bind(Transform & transform);
        static void Unbind();

    protected:
        Transform();

    private:
        friend class Manager::Physics::LerpManager;
        friend class Manager::Graphics::ShadowManager;
        friend class Manager::Graphics::Renderer;

        static WeakTransform FindNextTransform(const Transform& transform_);

        SERIALIZER_ACCESS

        bool m_b_absolute_;
        Vector3    m_previous_position_;
        Vector3    m_world_previous_position_;
        Vector3    m_position_;
        Vector3    m_yaw_pitch_roll_degree_;
        Quaternion m_rotation_;
        Vector3    m_scale_;

        Vector3 m_animation_position_;
        Quaternion m_animation_rotation_;
        Vector3 m_animation_scale_;

        // Non-serialized
        Graphics::CBs::TransformCB m_transform_buffer_;
    };
} // namespace Engine::Component

BOOST_CLASS_EXPORT_KEY(Engine::Components::Transform)
