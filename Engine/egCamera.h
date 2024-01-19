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
        m_zoom_(1.0f),
        m_b_orthogonal_(false),
        m_b_fixed_up_(true) {}

    ~Camera() override = default;

    void Initialize() override;
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void OnDeserialized() override;
    void OnImGui() override;

    void SetOrthogonal(bool bOrthogonal);
    void SetFixedUp(bool bFixedUp);
    void SetZoom(float zoom);

    Matrix  GetViewMatrix() const;
    Matrix  GetProjectionMatrix() const;
    Matrix  GetWorldMatrix() const;
    Vector2 GetWorldMousePosition();
    bool    GetOrthogonal() const;

  private:
    SERIALIZER_ACCESS

    float m_zoom_;
    bool m_b_orthogonal_;
    bool m_b_fixed_up_;

    // Non-serialized
    Matrix m_world_matrix_;
    Matrix m_view_matrix_;
    Matrix m_projection_matrix_;

    Graphics::CBs::PerspectiveCB m_wvp_buffer_;
  };
} // namespace Engine::Objects

BOOST_CLASS_EXPORT_KEY(Engine::Objects::Camera)
