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

        void __vectorcall SetWorldPosition(const Vector3& position);
        void __vectorcall SetWorldRotation(const Quaternion& rotation);
        void __vectorcall SetWorldScale(const Vector3& scale);

        void __vectorcall SetLocalPosition(const Vector3& position);
        void __vectorcall SetLocalRotation(const Quaternion& rotation);
        void __vectorcall SetLocalScale(const Vector3& scale);
        void __vectorcall SetLocalMatrix(const Matrix& matrix);
        void SetSizeAbsolute(bool absolute);
        void SetRotateAbsolute(bool absolute);

        void __vectorcall SetAnimationPosition(const Vector3& position);
        void __vectorcall SetAnimationRotation(const Quaternion& rotation);
        void __vectorcall SetAnimationScale(const Vector3& scale);
        void __vectorcall SetAnimationMatrix(const Matrix& matrix);

        Vector3    GetWorldPosition();
        Quaternion GetWorldRotation() const;
        Vector3    GetWorldScale() const;
        Vector3    GetWorldPreviousPosition() const;
        Vector3    GetLocalPreviousPosition() const;
        Vector3    GetLocalPosition() const;
        Quaternion GetLocalRotation() const;
        Vector3    GetLocalScale() const;
        Vector3    GetAnimationPosition() const;
        Vector3    GetAnimationScale() const;
        Quaternion GetAnimationRotation() const;

        Vector3 Forward() const;
        Vector3 Right() const;
        Vector3 Up() const;

        void Translate(Vector3 translation);

        void Initialize() override;
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PostUpdate(const float& dt) override;
        void FixedUpdate(const float& dt) override;

        void     OnImGui() override;
        void     OnDeserialized() override;

        Matrix GetLocalMatrix() const;
        Matrix GetWorldMatrix();

        static void __fastcall Bind(Transform & transform);
        static void Unbind();

    protected:
        Transform();

    private:
        friend class Manager::Physics::LerpManager;
        friend class Manager::Graphics::ShadowManager;
        friend class Manager::Graphics::Renderer;

        static WeakTransform FindNextTransform(const Transform& transform_);

        SERIALIZER_ACCESS

        bool m_b_s_absolute_;
        bool m_b_r_absolute_;
        Vector3    m_previous_position_;
        Vector3    m_world_previous_position_;
        Vector3    m_position_;
        Quaternion m_rotation_;
        Vector3    m_scale_;

        Vector3    m_animation_position_;
        Quaternion m_animation_rotation_;
        Vector3    m_animation_scale_;
        Matrix     m_animation_matrix_;

        // Non-serialized
        bool                       m_b_lazy_;
        Matrix                     m_world_matrix_;
        Graphics::CBs::TransformCB m_transform_buffer_;
    };
} // namespace Engine::Component

BOOST_CLASS_EXPORT_KEY(Engine::Components::Transform)
