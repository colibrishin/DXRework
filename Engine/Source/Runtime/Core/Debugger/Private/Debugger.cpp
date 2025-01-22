#include "../Public/Debugger.hpp"

#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.h"
#include "Source/Runtime/Core/Scene/Public/Scene.h"
#include "Source/Runtime/Core/Components/Transform/Public/Transform.h"
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

namespace Engine::Managers
{
	void Debugger::Render(const float dt) {}

	void Debugger::PreUpdate(const float dt) {}

	void Debugger::Update(const float dt)
	{	
#if WITH_DEBUG
		/*
		if (Managers::InputManager::GetInstance().GetCurrentKeyState().Scroll)
		{
			m_bDebug = !m_bDebug;
		}

		if (const auto& scene = Managers::SceneManager::GetInstance().GetActiveScene().lock())
		{
			if (const auto& cam = scene->GetMainCamera().lock())
			{
				// There could be multiple camera objects in the scene.
				// todo: change main camera
				if (!g_camera_lock)
				{
					int value = 0;

					if (Managers::InputManager::GetInstance().HasScrollChanged(value))
					{
						cam->GetComponent<Components::Transform>().lock()->Translate
								(Vector3::Up * -static_cast<float>(value));
					}

					if (Managers::InputManager::GetInstance().IsKeyPressed(Keyboard::W))
					{
						cam->GetComponent<Components::Transform>().lock()->Translate(-g_forward * g_camera_speed);
					}
					if (Managers::InputManager::GetInstance().IsKeyPressed(Keyboard::S))
					{
						cam->GetComponent<Components::Transform>().lock()->Translate(g_forward * g_camera_speed);
					}
					if (Managers::InputManager::GetInstance().IsKeyPressed(Keyboard::A))
					{
						cam->GetComponent<Components::Transform>().lock()->Translate
								(Vector3::Left * g_camera_speed);
					}
					if (Managers::InputManager::GetInstance().IsKeyPressed(Keyboard::D))
					{
						cam->GetComponent<Components::Transform>().lock()->Translate
								(Vector3::Right * g_camera_speed);
					}
				}
			}
		}
		*/
#endif
	}

	void Debugger::PreRender(const float dt) {}

	void Debugger::FixedUpdate(const float dt) {}

	void Debugger::PostRender(const float dt) { }

	void Debugger::PostUpdate(const float dt)
	{
#if WITH_DEBUG
		if (m_render_queue_.empty())
		{
			return;
		}

		while (m_render_queue_.size() > CFG_DEBUG_MAX_MESSAGE)
		{
			m_render_queue_.pop_front();
		}

		for (auto it = m_render_queue_.begin(); it != m_render_queue_.end();)
		{
			if (it->elapsed_time > CFG_DEBUG_MESSAGE_LIFETIME)
			{
				it = m_render_queue_.erase(it);
			}
			else
			{
				++it;
			}
		}

		for (auto it = m_render_queue_.begin(); it != m_render_queue_.end(); ++it)
		{
			it->elapsed_time += dt;
			CallbackMessage(dt, it->type, *it);
		}
#endif
	}

	Debugger::Debugger(SINGLETON_LOCK_TOKEN)
		: Singleton() {}

	void Debugger::Initialize() {}

	void Debugger::Log(const std::string& str, const Color& color)
	{
		Message msg{};
		msg.type = DEBUG_MSG_LOG;
		msg.elapsed_time = 0.f;
		msg.x = m_x_;
		msg.y = m_y_;
		msg.text = str;
		msg.color = color;

		Push(msg);
		m_y_ += CFG_DEBUG_MESSAGE_Y_MOVEMENT;
		m_y_ = std::fmod(m_y_, CFG_HEIGHT);
	}

	void Debugger::Draw(const Vector3& start, const Vector3& end, const Color& color)
	{
		Message msg{};
		msg.type = DEBUG_MSG_LINE;
		msg.elapsed_time = 0.f;
		msg.ray_start = start;
		msg.ray_end = end;
		msg.color = color;
		
		Push(msg);
	}

	void Debugger::Draw(const Ray& ray, const Color& color)
	{
		Message msg{};
		msg.type = DEBUG_MSG_RAY;
		msg.elapsed_time = 0.f;
		msg.ray = ray;
		msg.color = color;
		
		Push(msg);
	}

	void Debugger::SetCallback(const eDebugMessage type, const DebugCallback& callback)
	{
#if WITH_DEBUG
		m_process_functions_[type] = callback;
#endif
	}

	void Debugger::Draw(const BoundingFrustum& frustum, const Color& color)
	{
		Message msg{};
		msg.type = DEBUG_MSG_FRUSTUM;
		msg.elapsed_time = 0.f;
		msg.frustum = frustum;
		msg.color = color;
		
		Push(msg);
	}

	void Debugger::Draw(const BoundingSphere& sphere, const Color& color)
	{
		Message msg{};
		msg.type = DEBUG_MSG_SPHERE;
		msg.elapsed_time = 0.f;
		msg.sphere = sphere;
		msg.color = color;
		
		Push(msg);
	}

	void Debugger::Draw(const BoundingOrientedBox& obb, const Color& color)
	{
		Message msg{};
		msg.type = DEBUG_MSG_OBB;
		msg.elapsed_time = 0.f;
		msg.obb = obb;
		msg.color = color;
		
		Push(msg);
	}

	void Debugger::Draw(const BoundingBox& bb, const Color& color)
	{
		Message msg{};
		msg.type = DEBUG_MSG_AABB;
		msg.elapsed_time = 0.f;
		msg.aabb = bb;
		msg.color = color;
		
		Push(msg);
	}

	void Debugger::Push(const Message& msg)
	{
#if WITH_DEBUG
		if (m_render_queue_.size() > CFG_DEBUG_MAX_MESSAGE)
		{
			m_render_queue_.pop_front();
		}

		m_render_queue_.emplace_back(msg);
#else
		return;
#endif
	}
} // namespace Engine::Manager
