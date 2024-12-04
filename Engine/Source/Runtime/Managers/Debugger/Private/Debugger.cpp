#include "../Public/Debugger.hpp"
#include "../Public/DebugDraw.h"

#include "Source/Runtime/Managers/SceneManager/Public/SceneManager.hpp"
#include "Source/Runtime/Managers/D3D12Toolkit/Public/ToolkitAPI.h"
#include "Source/Runtime/Scene/Public/Scene.hpp"
#include "Source/Runtime/Components/Transform/Public/Transform.h"
#include "Source/Runtime/CoreObjects/Camera/Public/Camera.h"

namespace Engine::Managers
{
	void Debugger::Render(const float& dt) {}

	void Debugger::PreUpdate(const float& dt) {}

	void Debugger::Update(const float& dt)
	{	
#if WITH_DEBUG
		if (GetApplication().GetCurrentKeyState().Scroll)
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

					if (GetApplication().HasScrollChanged(value))
					{
						cam->GetComponent<Components::Transform>().lock()->Translate
								(Vector3::Up * -static_cast<float>(value));
					}

					if (GetApplication().IsKeyPressed(Keyboard::W))
					{
						cam->GetComponent<Components::Transform>().lock()->Translate(-g_forward * g_camera_speed);
					}
					if (GetApplication().IsKeyPressed(Keyboard::S))
					{
						cam->GetComponent<Components::Transform>().lock()->Translate(g_forward * g_camera_speed);
					}
					if (GetApplication().IsKeyPressed(Keyboard::A))
					{
						cam->GetComponent<Components::Transform>().lock()->Translate
								(Vector3::Left * g_camera_speed);
					}
					if (GetApplication().IsKeyPressed(Keyboard::D))
					{
						cam->GetComponent<Components::Transform>().lock()->Translate
								(Vector3::Right * g_camera_speed);
					}
				}
			}
		}
#endif
	}

	void Debugger::PreRender(const float& dt) {}

	void Debugger::FixedUpdate(const float& dt) {}

	void Debugger::PostRender(const float& dt)
	{
		if (!GetDebugFlag())
		{
			return;
		}

		if (m_render_queue.empty())
		{
			return;
		}

		while (m_render_queue.size() > CFG_DEBUG_MAX_MESSAGE)
		{
			m_render_queue.pop_front();
		}

		for (auto it = m_render_queue.begin(); it != m_render_queue.end();)
		{
			if (it->first.elapsed_time > CFG_DEBUG_MESSAGE_LIFETIME)
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
				Managers::ToolkitAPI::GetInstance().AppendPrimitiveBatch
						(
						 [this, it, dt]()
						 {
							 it->second(it->first, dt);
						 }
						);
				break;
			case TOOLKIT_RENDER_SPRITE:
				Managers::ToolkitAPI::GetInstance().AppendSpriteBatch
						(
						 [this, it, dt]()
						 {
							 it->second(it->first, dt);
						 }
						);
				break;
			case TOOLKIT_RENDER_UNKNOWN:
			default: ;
			}
		}

		y = CFG_DEBUG_MESSAGE_Y_MOVEMENT;
	}

	void Debugger::PostUpdate(const float& dt) {}

	Debugger::Debugger(SINGLETON_LOCK_TOKEN)
		: Singleton(),
		  m_bDebug(false) {}

	void Debugger::Initialize()
	{
		auto upload_batch = DirectX::ResourceUploadBatch(Managers::D3Device::GetInstance().GetDevice());

		upload_batch.Begin();

		m_bDebug = true;
		m_font_  = std::make_unique<DirectX::SpriteFont>
				(
				 Managers::D3Device::GetInstance().GetDevice(),
				 upload_batch,
				 L"consolas.spritefont",
				 Managers::ToolkitAPI::GetInstance().GetDescriptorHeap()->GetCpuHandle(0),
				 Managers::ToolkitAPI::GetInstance().GetDescriptorHeap()->GetGpuHandle(0)
				);

		const auto& token = upload_batch.End(Managers::D3Device::GetInstance().GetCommandQueue(COMMAND_LIST_UPDATE));

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
							  Managers::ToolkitAPI::GetInstance().GetSpriteBatch(), msg.log.c_str(),
							  XMFLOAT2(static_cast<float>(x), static_cast<float>(y)),
							  DirectX::Colors::OrangeRed, 0.0f, Vector2::Zero, 0.5f
							 );

					 y += CFG_DEBUG_MESSAGE_Y_MOVEMENT;
					 y %= CFG_HEIGHT;
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
					 DX::DrawRay(Managers::ToolkitAPI::GetInstance().GetPrimitiveBatch(), start, end, false, color);
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
							  Managers::ToolkitAPI::GetInstance().GetPrimitiveBatch(), ray.position,
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
					 DX::Draw(Managers::ToolkitAPI::GetInstance().GetPrimitiveBatch(), frustum, color);
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
					 DX::Draw(Managers::ToolkitAPI::GetInstance().GetPrimitiveBatch(), sphere, color);
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
					 DX::Draw(Managers::ToolkitAPI::GetInstance().GetPrimitiveBatch(), obb, color);
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
					 DX::Draw(Managers::ToolkitAPI::GetInstance().GetPrimitiveBatch(), bb, color);
					 msg.elapsed_time += 2.f;
				 }
				);
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
		const std::function<void(Message&, float)>& func
	)
	{
		if (!m_bDebug)
		{
			return;
		}

		if (m_render_queue.size() > CFG_DEBUG_MAX_MESSAGE)
		{
			m_render_queue.pop_front();
		}

		m_render_queue.emplace_back(msg, func);
	}
} // namespace Engine::Manager
