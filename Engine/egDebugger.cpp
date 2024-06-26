#include "pch.h"
#include "egDebugger.hpp"

#include <DescriptorHeap.h>

#include "egCamera.h"
#include "egGlobal.h"
#include "egSceneManager.hpp"
#include "egTransform.h"

namespace Engine::Manager
{
  void Debugger::Render(const float& dt)
  {
    OnImGui();
  }

  void Debugger::PreUpdate(const float& dt) {}

  void Debugger::Update(const float& dt)
  {
    if constexpr (g_debug)
    {
      if (GetApplication().GetCurrentKeyState().Scroll)
      {
        m_bDebug = !m_bDebug;
      }

      if (const auto& scene = GetSceneManager().GetActiveScene().lock())
      {
        if (const auto& cam = scene->GetMainCamera().lock())
        {
          // There could be multiple camera objects in the scene.
          // todo: change main camera
          if (!g_camera_lock)
          {
            int value = 0;

            if (GetApplication().HasScrollChanged(value))
            {
              cam->GetComponent<Components::Transform>().lock()->Translate(Vector3::Up * -value);
            }

            if (GetApplication().IsKeyPressed(Keyboard::W))
            {
              cam->GetComponent<Components::Transform>().lock()->Translate(-g_forward * g_camera_speed);
            }
            if (GetApplication().IsKeyPressed(Keyboard::S))
            {
              cam->GetComponent<Components::Transform>().lock()->Translate(g_forward* g_camera_speed);
            }
            if (GetApplication().IsKeyPressed(Keyboard::A))
            {
              cam->GetComponent<Components::Transform>().lock()->Translate(Vector3::Left* g_camera_speed);
            }
            if (GetApplication().IsKeyPressed(Keyboard::D))
            {
              cam->GetComponent<Components::Transform>().lock()->Translate(Vector3::Right* g_camera_speed);
            }
          }
        }
      }
    }
  }

  void Debugger::PreRender(const float& dt) {}

  void Debugger::FixedUpdate(const float& dt) {}

  void Debugger::PostRender(const float& dt)
  {
    if (!GetDebugFlag()) { return; }

    if (m_render_queue.empty()) { return; }

    while (m_render_queue.size() > g_debug_message_max)
    {
      m_render_queue.pop_front();
    }

    for (auto it = m_render_queue.begin(); it != m_render_queue.end();)
    {
      if (it->first.elapsed_time > g_debug_message_life_time)
      {
        it = m_render_queue.erase(it);
      }
      else
      {
        ++it;
      }
    }

    for (auto it = m_render_queue.begin(); it != m_render_queue.end(); ++it)
    {
      switch (it->first.redirection)
      {
      case TOOLKIT_RENDER_PRIMITIVE: 
          GetToolkitAPI().AppendPrimitiveBatch([this, it, dt]()
          {
            it->second(it->first, dt);
          });
        break;
      case TOOLKIT_RENDER_SPRITE: 
          GetToolkitAPI().AppendSpriteBatch([this, it, dt]()
          {
            it->second(it->first, dt);
          });
          break;
      case TOOLKIT_RENDER_UNKNOWN:
      default: ;
      }
    }

    y = g_debug_y_initial;
  }

  void Debugger::PostUpdate(const float& dt) {}

  void Debugger::OnImGui()
  {
    if constexpr (g_debug)
    {
      if (ImGui::Begin("Debugger"))
      {
        if (ImGui::Button("Clear"))
        {
          m_render_queue.clear();
        }

        ImGui::SameLine();
        if (ImGui::Button("Toggle"))
        {
          m_bDebug = !m_bDebug;
        }

        static auto pause_button_string  = "Pause";
        static auto resume_button_string = "Resume";
        if (ImGui::Button(g_paused ? resume_button_string : pause_button_string))
        {
          g_paused = !g_paused;
        }

        static auto free_camera_button = "Free Camera";
        static auto lock_camera_button = "Lock Camera";
        ImGui::SameLine();
        if (ImGui::Button(g_camera_lock ? free_camera_button : lock_camera_button))
        {
          g_camera_lock = !g_camera_lock;
        }

        ImGui::DragFloat("Camera speed", &g_camera_speed, 0.1, 0, 1);

        ImGui::End();
      }
    }
  }

  Debugger::Debugger(SINGLETON_LOCK_TOKEN)
    : Singleton(),
      m_bDebug(false) {}

  void Debugger::Initialize()
  {
    auto upload_batch = DirectX::ResourceUploadBatch(GetD3Device().GetDevice());

    upload_batch.Begin();

    m_bDebug = true;
    m_font_  = std::make_unique<SpriteFont>
      (
       GetD3Device().GetDevice(),
       upload_batch,
       L"consolas.spritefont",
       GetToolkitAPI().GetDescriptorHeap()->GetCpuHandle(0),
       GetToolkitAPI().GetDescriptorHeap()->GetGpuHandle(0)
      );

    const auto& token = upload_batch.End(GetD3Device().GetCommandQueue(COMMAND_LIST_UPDATE));

    token.wait();
  }

  void Debugger::Log(const std::string& str)
  {
    Push
      (
       Message{str, TOOLKIT_RENDER_SPRITE}, [&](Message& msg, const float& dt)
       {
         m_font_->DrawString
           (
            GetToolkitAPI().GetSpriteBatch(), msg.log.c_str(),
            XMFLOAT2(static_cast<float>(x), static_cast<float>(y)),
            DirectX::Colors::OrangeRed, 0.0f, Vector2::Zero, 0.5f
           );

         y += g_debug_y_movement;
         y %= g_window_height;
         msg.elapsed_time += dt;
       }
      );
  }

  void Debugger::Draw(
    const Vector3&     start, const Vector3& end,
    const XMVECTORF32& color
  )
  {
    Push
      (
       Message{"", TOOLKIT_RENDER_PRIMITIVE}, [start, end, color](Message& msg, const float& dt)
       {
         DX::DrawRay(GetToolkitAPI().GetPrimitiveBatch(), start, end, false, color);
         msg.elapsed_time += dt;
       }
      );
  }

  void Debugger::Draw(Ray& ray, const XMVECTORF32& color)
  {
    Push
      (
       Message{"", TOOLKIT_RENDER_PRIMITIVE}, [ray, color](Message& msg, const float& dt)
       {
         DX::DrawRay
           (
            GetToolkitAPI().GetPrimitiveBatch(), ray.position,
            ray.direction, true, color
           );
         msg.elapsed_time += dt;
       }
      );
  }

  void Debugger::Draw(const BoundingFrustum& frustum, const XMVECTORF32& color)
  {
    Push
      (
       Message{"", TOOLKIT_RENDER_PRIMITIVE}, [frustum, color](Message& msg, const float& dt)
       {
         DX::Draw(GetToolkitAPI().GetPrimitiveBatch(), frustum, color);
         msg.elapsed_time += 2.f;
       }
      );
  }

  void Debugger::Draw(const BoundingSphere& sphere, const XMVECTORF32& color)
  {
    Push
      (
       Message{"", TOOLKIT_RENDER_PRIMITIVE}, [sphere, color](Message& msg, const float& dt)
       {
         DX::Draw(GetToolkitAPI().GetPrimitiveBatch(), sphere, color);
         msg.elapsed_time += 2.f;
       }
      );
  }

  void Debugger::Draw(const BoundingOrientedBox& obb, const XMVECTORF32& color)
  {
    Push
      (
       Message{"", TOOLKIT_RENDER_PRIMITIVE}, [obb, color](Message& msg, const float& dt)
       {
         DX::Draw(GetToolkitAPI().GetPrimitiveBatch(), obb, color);
         msg.elapsed_time += 2.f;
       }
      );
  }

  void Debugger::Draw(const BoundingBox& bb, const XMVECTORF32& color)
  {
    Push
      (
       Message{"", TOOLKIT_RENDER_PRIMITIVE}, [bb, color](Message& msg, const float& dt)
       {
         DX::Draw(GetToolkitAPI().GetPrimitiveBatch(), bb, color);
         msg.elapsed_time += 2.f;
       }
      );
  }

  void Debugger::SetDebugFlag() { m_bDebug = true; }

  bool Debugger::GetDebugFlag() const { return m_bDebug; }

  void Debugger::Push(
    const Message&                              msg,
    const std::function<void(Message&, float)>& func
  )
  {
    if (!m_bDebug) { return; }

    if (m_render_queue.size() > g_debug_message_max)
    {
      m_render_queue.pop_front();
    }

    m_render_queue.emplace_back(msg, func);
  }
} // namespace Engine::Manager
