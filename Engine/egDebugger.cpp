#include "pch.h"
#include "egDebugger.hpp"

#include "egGlobal.h"

namespace Engine::Manager
{
    void Debugger::Render(const float& dt) {}

    void Debugger::PreUpdate(const float& dt) {}

    void Debugger::Update(const float& dt)
    {
        if (GetApplication().GetKeyState().Scroll)
        {
            m_bDebug = !m_bDebug;
        }
    }

    void Debugger::PreRender(const float& dt) {}

    void Debugger::FixedUpdate(const float& dt) {}

    void Debugger::PostRender(const float& dt)
    {
        if (!GetDebugFlag())
        {
            return;
        }

        GetToolkitAPI().BeginPrimitiveBatch();

        if (m_render_queue.size() > g_debug_message_max)
        {
            while (m_render_queue.size() > g_debug_message_max)
            {
                m_render_queue.erase(m_render_queue.begin());
            }
        }

        for (int i = 0; i < m_render_queue.size(); ++i)
        {
            if (m_render_queue[i].first.elapsed_time > g_debug_message_life_time)
            {
                m_render_queue.erase(m_render_queue.begin() + i);
                continue;
            }

            m_render_queue[i].second(m_render_queue[i].first, dt);
        }

        y = g_debug_y_initial;

        GetToolkitAPI().EndPrimitiveBatch();
    }

    void Debugger::PostUpdate(const float& dt) {}

    Debugger::Debugger(SINGLETON_LOCK_TOKEN)
    : Singleton(),
      m_bDebug(false) {}

    void Debugger::Initialize()
    {
        m_bDebug = true;
        m_font_  = std::make_unique<SpriteFont>(
                                                GetD3Device().GetDevice(),
                                                L"consolas.spritefont");
    }

    void Debugger::Log(const std::string& str)
    {
        Push(
             Message{str}, [&](Message& msg, const float& dt)
             {
                 m_font_->DrawString(
                                     GetToolkitAPI().GetSpriteBatch(), msg.log.c_str(),
                                     XMFLOAT2(static_cast<float>(x), static_cast<float>(y)),
                                     DirectX::Colors::OrangeRed, 0.0f, Vector2::Zero, 0.5f);

                 y += g_debug_y_movement;
                 y %= g_window_height;
                 msg.elapsed_time += dt;
             });
    }

    void Debugger::Draw(
        const Vector3&     start, const Vector3& end,
        const XMVECTORF32& color)
    {
        Push(
             Message{}, [start, end, color](Message& msg, const float& dt)
             {
                 DX::DrawRay(GetToolkitAPI().GetPrimitiveBatch(), start, end, false, color);
                 msg.elapsed_time += dt;
             });
    }

    void Debugger::Draw(Ray& ray, const XMVECTORF32& color)
    {
        Push(
             Message{}, [ray, color](Message& msg, const float& dt)
             {
                 DX::DrawRay(
                             GetToolkitAPI().GetPrimitiveBatch(), ray.position,
                             ray.direction, true, color);
                 msg.elapsed_time += dt;
             });
    }

    void Debugger::Draw(const BoundingFrustum& frustum, const XMVECTORF32& color)
    {
        Push(
             Message{}, [frustum, color](Message& msg, const float& dt)
             {
                 DX::Draw(GetToolkitAPI().GetPrimitiveBatch(), frustum, color);
                 msg.elapsed_time += 2.f;
             });
    }

    void Debugger::Draw(const BoundingSphere& sphere, const XMVECTORF32& color)
    {
        Push(
             Message{}, [sphere, color](Message& msg, const float& dt)
             {
                 DX::Draw(GetToolkitAPI().GetPrimitiveBatch(), sphere, color);
                 msg.elapsed_time += 2.f;
             });
    }

    void Debugger::Draw(const BoundingOrientedBox& obb, const XMVECTORF32& color)
    {
        Push(
             Message{}, [obb, color](Message& msg, const float& dt)
             {
                 DX::Draw(GetToolkitAPI().GetPrimitiveBatch(), obb, color);
                 msg.elapsed_time += 2.f;
             });
    }

    void Debugger::Draw(const BoundingBox& bb, const XMVECTORF32& color)
    {
        Push(
             Message{}, [bb, color](Message& msg, const float& dt)
             {
                 DX::Draw(GetToolkitAPI().GetPrimitiveBatch(), bb, color);
                 msg.elapsed_time += 2.f;
             });
    }

    void Debugger::SetDebugFlag()
    {
        m_bDebug = true;
    }

    bool Debugger::GetDebugFlag() const
    {
        return m_bDebug;
    }

    void Debugger::Push(
        const Message&                              msg,
        const std::function<void(Message&, float)>& func)
    {
        if (!m_bDebug)
        {
            return;
        }

        m_render_queue.emplace_back(msg, func);
    }
} // namespace Engine::Manager
