#pragma once
#include "egManager.hpp"

namespace Engine::Manager
{
  class MouseManager : public Abstract::Singleton<MouseManager>
  {
  public:
    MouseManager(SINGLETON_LOCK_TOKEN)
      : Singleton<MouseManager>() {};
    void Initialize() override;
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PostUpdate(const float& dt) override;

    static Vector2 GetNormalizedMousePosition();

    [[nodiscard]] const Quaternion& GetMouseRotation() const;
    [[nodiscard]] const Quaternion& GetMouseXRotation() const;
    [[nodiscard]] const Quaternion& GetMouseYRotation() const;

  private:
    friend struct SingletonDeleter;
    ~MouseManager() override = default;

    Quaternion m_mouse_rot_x_;
    Quaternion m_mouse_rot_y_;

    Vector2 m_previous_mouse_position_;
    Vector2 m_current_mouse_position_;
  };
} // namespace Engine::Manager
