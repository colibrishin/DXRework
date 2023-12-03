#pragma once
#include <memory>
#include <SimpleMath.h>

#include "egCommon.hpp"
#include "egObject.hpp"

namespace Engine::Objects
{
	class Camera final : public Abstract::Object
	{
	public:
		Camera() : Object() {}
		~Camera() override = default;

		void SetLookAt(Vector3 lookAt)
		{
			m_look_at_ = lookAt;
		}

		void SetPosition(Vector3 position);
		void SetRotation(Quaternion rotation);

		Vector3 GetPosition();
		Matrix GetViewMatrix() const { return m_view_matrix_; }
		Quaternion GetMouseRotation() const { return m_mouse_rotation_; }
		Vector3 GetLookAt() const;

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void FixedUpdate(const float& dt) override;

		void BindObject(const WeakObject& object);
		void SetOffset(Vector3 offset);

		Vector2 GetWorldMousePosition();
		static Vector2 GetNormalizedMousePosition();

	private:
		friend class boost::serialization::access;
		Matrix m_view_matrix_;
		VPBuffer m_vp_buffer_;

		Vector3 m_look_at_;
		Quaternion m_mouse_rotation_;
		Matrix m_mouse_rotation_matrix_;

		Vector2 m_previous_mouse_position_;
		Vector2 m_current_mouse_position_;

		Vector3 m_offset_;
		WeakObject m_bound_object_;
	};
}
