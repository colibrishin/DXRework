#pragma once
#include "egCommon.hpp"
#include "egObject.hpp"

namespace Engine::Objects
{
    class Camera final : public Abstract::Object
    {
    public:
        OBJECT_T(DEF_OBJ_T_CAMERA)

        Camera()
        : Object(DEF_OBJ_T_CAMERA),
          m_bound_object_id_(g_invalid_id),
          m_b_orthogonal_(false) {}

        ~Camera() override = default;

        void SetLookAt(Vector3 lookAt);
        void SetPosition(Vector3 position);
        void SetRotation(Quaternion rotation);
        void SetLookAtRotation(Quaternion rotation);

        Quaternion GetRotation();
        Vector3    GetPosition();

        Quaternion GetLookAtRotation() const;
        Matrix     GetViewMatrix() const;
        Matrix     GetProjectionMatrix() const;
        Matrix     GetWorldMatrix() const;
        Vector3    GetLookAt() const;

        void Initialize() override;
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void OnDeserialized() override;
        void OnImGui() override;

        void BindObject(const WeakObject& object);
        void SetOffset(Vector3 offset);

        void SetOrthogonal(bool bOrthogonal);
        Vector2 GetWorldMousePosition();
        bool GetOrthogonal() const;

    private:
        SERIALIZER_ACCESS

        Vector3    m_look_at_;
        Quaternion m_look_at_rotation_;

        Vector3 m_offset_;
        LocalActorID m_bound_object_id_;

        bool m_b_orthogonal_;

        // Non-serialized
        Matrix m_world_matrix_;
        Matrix m_view_matrix_;
        Matrix m_projection_matrix_;

        Graphics::CBs::PerspectiveCB m_wvp_buffer_;
        WeakObject        m_bound_object_;
    };
} // namespace Engine::Objects

BOOST_CLASS_EXPORT_KEY(Engine::Objects::Camera)
