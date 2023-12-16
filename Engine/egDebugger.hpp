#pragma once
#include <Windows.h>
#include <memory>
#include <queue>

#include <SpriteFont.h>
#include "egApplication.h"
#include "egCommon.hpp"
#include "egD3Device.hpp"
#include "egToolkitAPI.h"

#include "DebugDraw.h"

namespace Engine::Manager
{
    class Debugger final : public Abstract::Singleton<Debugger>
    {
    public:
        explicit Debugger(SINGLETON_LOCK_TOKEN);

        ~Debugger() override = default;

        void Initialize() override;

        void Log(const std::wstring& str);
        void Draw(const Vector3& start, const Vector3& end, const XMVECTORF32& color);
        void Draw(Ray& ray, const XMVECTORF32& color);
        void Draw(const BoundingFrustum& frustum, const XMVECTORF32& color);
        void Draw(const BoundingSphere& sphere, const XMVECTORF32& color);
        void Draw(const BoundingOrientedBox& obb, const XMVECTORF32& color);

        void SetDebugFlag();
        bool GetDebugFlag() const;

        void Render(const float& dt) override;
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void PreRender(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PostRender(const float& dt) override;

    private:
        struct Message
        {
            std::wstring log;
            float        elapsed_time;
        };

        using DebugPair = std::pair<Message, std::function<void(Message&, float)>>;

        void Push(
            const Message&                              msg,
            const std::function<void(Message&, float)>& func);

    private:
        bool m_bDebug;
        int  x = 0;
        int  y = g_debug_y_initial;

        std::unique_ptr<SpriteFont> m_font_;

        std::vector<DebugPair> m_render_queue;
    };
} // namespace Engine::Manager
