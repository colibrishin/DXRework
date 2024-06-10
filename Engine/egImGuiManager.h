#pragma once
#include "egManager.hpp"

namespace Engine::Manager::Graphics
{
  class ImGuiManager final : public Abstract::Singleton<ImGuiManager, HWND>
  {
  public:
    explicit ImGuiManager(SINGLETON_LOCK_TOKEN) {}

    void Initialize(HWND hwnd) override;
    void PreUpdate(const float& dt) override;
    void Update(const float& dt) override;
    void PreRender(const float& dt) override;
    void Render(const float& dt) override;
    void PostRender(const float& dt) override;
    void FixedUpdate(const float& dt) override;
    void PostUpdate(const float& dt) override;

    void NewFrame() const;

  private:
    friend struct SingletonDeleter;
    ~ImGuiManager() override;

    // ImGui Graphics
    DescriptorPtr m_imgui_descriptor_;
  };
} // namespace Engine::Manager::Graphics
