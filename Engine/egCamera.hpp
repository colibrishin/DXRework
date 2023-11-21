#pragma once
#include <memory>
#include <SimpleMath.h>

#include "egCommon.hpp"
#include "egObject.hpp"
#include "egTransform.hpp"

namespace Engine::Objects
{
	class Camera final : public Abstract::Object
	{
	public:
		Camera() = default;
		~Camera() override = default;

		void SetLookAt(Vector3 lookAt)
		{
			m_look_at_ = lookAt;
		}

		void SetPosition(Vector3 position)
		{
			GetComponent<Component::Transform>().lock()->SetPosition(position);
		}

		void SetRotation(Quaternion rotation)
		{
			GetComponent<Component::Transform>().lock()->SetRotation(rotation);
		}

		Matrix GetViewMatrix() const { return m_view_matrix_; }
		Vector3 GetLookAt() const { return m_look_at_; }

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;

		void BindObject(const WeakObject& object);
		void SetOffset(Vector3 offset);

		Vector2 GetWorldMousePosition();

	private:
		Matrix m_view_matrix_;
		VPBuffer m_vp_buffer_;

		Vector3 m_look_at_;

		Vector3 m_offset_;
		WeakObject m_bound_object_;
	};
}
