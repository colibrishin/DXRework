#include "pch.h"
#include "egImGuiManager.h"

namespace Engine::Manager::Graphics
{
  void ImGuiManager::Initialize(HWND hwnd)
  {
    m_imgui_descriptor_ = GetRenderPipeline().AcquireHeapSlot().lock();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX12_Init
      (
       GetD3Device().GetDevice(), g_frame_buffer, DXGI_FORMAT_R8G8B8A8_UNORM,
       m_imgui_descriptor_->GetMainDescriptorHeap(),
       m_imgui_descriptor_->GetCPUHandle(),
       m_imgui_descriptor_->GetGPUHandle()
      );
  }

  void ImGuiManager::PreUpdate(const float& dt) {}

  void ImGuiManager::PreRender(const float& dt) {}

  void ImGuiManager::Render(const float& dt) {}

  void ImGuiManager::PostRender(const float& dt)
  {
    ImGui::Render();
    
    if (!GetD3Device().IsCommandPairAvailable())
    {
      throw std::runtime_error("Command Pair is not available for ImGui Rendering");
    }

    const auto& cmd = GetD3Device().AcquireCommandPair(L"ImGui Rendering").lock();

    cmd->SoftReset();
    GetRenderPipeline().DefaultRenderTarget(cmd);
    GetRenderPipeline().DefaultScissorRect(cmd);
    GetRenderPipeline().DefaultViewport(cmd);
    
    m_imgui_descriptor_->BindGraphic(cmd);

    ImGui_ImplDX12_RenderDrawData
    (
        ImGui::GetDrawData(),
        cmd->GetList()
    );

    cmd->FlagReady();
  }

  void ImGuiManager::FixedUpdate(const float& dt) {}

  void ImGuiManager::PostUpdate(const float& dt) {}

  void ImGuiManager::NewFrame() const
  {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
  }

  ImGuiManager::~ImGuiManager()
  {
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
  }

  void ImGuiManager::Update(const float& dt) {}
}