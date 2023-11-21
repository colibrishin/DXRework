#pragma once
#include <Windows.h>
#include <queue>
#include <memory>

#include <SpriteFont.h>
#include "egD3Device.hpp"
#include "egCommon.hpp"
#include "egApplication.hpp"

namespace Engine::Manager
{
	class Debugger final : public Abstract::Singleton<Debugger>
	{
	public:
		explicit Debugger(SINGLETON_LOCK_TOKEN) : Singleton(), m_bDebug(false) {}
		~Debugger() override = default;

		void Initialize() override
		{
			m_bDebug = true;
			m_font_ = std::make_unique<SpriteFont>(GetD3Device().GetDevice(), L"consolas.spritefont");
		}

		void Log(const std::wstring& str)
		{
			Push(Message{str}, [&](Message& msg, const float& dt)
			{
				m_font_->DrawString(
					GetToolkitAPI().GetSpriteBatch(),
					msg.log.c_str(),
					DirectX::XMFLOAT2(static_cast<float>(x), static_cast<float>(y)),
					DirectX::Colors::OrangeRed,
					0.0f,
					Vector2::Zero,
					0.5f);

				y += g_debug_y_movement;
				y %= g_window_height;
				msg.elapsed_time += dt;
			});
		}

		void SetDebugFlag();
		bool GetDebugFlag() const;

		void Render(const float& dt) override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void FixedUpdate(const float& dt) override;

	private:
		struct Message
		{
			std::wstring log;
			float elapsed_time;
		};

		using DebugPair = std::pair<Message, std::function<void(Message&, float)>>;

		void Push(const Message& msg, const std::function<void(Message&, float)>& func);

		bool m_bDebug;
		int x = 0;
		int y = g_debug_y_initial;

		std::unique_ptr<SpriteFont> m_font_;

		std::vector<DebugPair> m_render_queue;
	};

	inline void Debugger::Render(const float& dt)
	{
		if(!GetDebugFlag())
		{
			return;
		}

		if (m_render_queue.size() > g_debug_message_max)
		{
			m_render_queue.erase(m_render_queue.begin());
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
	}

	inline void Debugger::PreUpdate(const float& dt)
	{
	}

	inline void Debugger::Update(const float& dt)
	{
		if (GetApplication().GetKeyState().Scroll)
		{
			m_bDebug = !m_bDebug;
		}
	}

	inline void Debugger::PreRender(const float& dt)
	{
	}

	inline void Debugger::FixedUpdate(const float& dt)
	{
	}

	inline void Debugger::SetDebugFlag()
	{
		m_bDebug = true;
	}

	inline bool Debugger::GetDebugFlag() const
	{
		return m_bDebug;
	}

	inline void Debugger::Push(const Message& msg, const std::function<void(Message&, float)>& func)
	{
		if (!m_bDebug)
		{
			return;
		}

		m_render_queue.emplace_back(msg, func);
	}
}
