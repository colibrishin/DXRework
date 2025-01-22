#pragma once
#include <deque>
#include <memory>

#include "Source/Runtime/CoreSingleton/Public/Singleton.hpp"
#include "TypeLibrary/Public/TypeLibrary.h"

namespace Engine
{
	enum CORE_API eDebugMessage : uint8_t
	{
		DEBUG_MSG_LOG,
		DEBUG_MSG_LINE,
		DEBUG_MSG_RAY,
		DEBUG_MSG_FRUSTUM,
		DEBUG_MSG_SPHERE,
		DEBUG_MSG_OBB,
		DEBUG_MSG_AABB,
		DEBUG_MSG_MAX
	};
	
	struct CORE_API Message
	{
		eDebugMessage type = DEBUG_MSG_MAX;
		float elapsed_time = 0.f;
		float x = 0.f;
		float y = 0.f;

		std::string         text;
		union
		{
			Color               color;
			BoundingBox         aabb;
			BoundingOrientedBox obb;
			BoundingSphere      sphere;
			BoundingFrustum     frustum;
			Ray                 ray;
			Vector3             ray_start;
			Vector3				ray_end;
		};
	};
		
	using DebugCallback = std::function<void(const Message&)>;
}

namespace Engine::Managers
{
	class CORE_API Debugger final : public Abstracts::Singleton<Debugger>
	{
	public:
		explicit Debugger(SINGLETON_LOCK_TOKEN);
		void     Initialize() override;

		void Log(const std::string& str, const Color& color);
		void Draw(const Vector3& start, const Vector3& end, const Color& color);
		void Draw(const BoundingBox& bb, const Color& color);
		void Draw(const BoundingOrientedBox& obb, const Color& color);
		void Draw(const BoundingSphere& sphere, const Color& color);
		void Draw(const BoundingFrustum& frustum, const Color& color);
		void Draw(const Ray& ray, const Color& color);

		void               SetCallback(eDebugMessage type, const DebugCallback& callback);
		void               SetDebugFlag();
		[[nodiscard]] bool GetDebugFlag() const;

		void Render(const float dt) override;
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void PreRender(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostRender(const float dt) override;
		void PostUpdate(const float dt) override;

	private:
		friend struct SingletonDeleter;
		~Debugger() override = default;

		void CallbackMessage(const float dt, const eDebugMessage type, Message& message) const
		{
			if (m_process_functions_[type])
			{
				m_process_functions_[type](message);
				message.elapsed_time += dt;
			}
		}
		
		void Push(const Message& msg);

		bool m_b_debug_;
		float m_x_ = 0.f;
		float m_y_ = CFG_DEBUG_MESSAGE_Y_MOVEMENT;

		DebugCallback       m_process_functions_[DEBUG_MSG_MAX]{};
		std::deque<Message> m_render_queue_;
	};
} // namespace Engine::Managers
