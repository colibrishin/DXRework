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
		Camera() : Object(), m_bound_object_id_(g_invalid_id), m_b_orthogonal_(false)
		{
		}

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
		void OnDeserialized() override;

		void BindObject(const WeakObject& object);
		void SetOffset(Vector3 offset);
		void SetOrthogonal(bool bOrthogonal) { m_b_orthogonal_ = bOrthogonal; }

		Vector2 GetWorldMousePosition();
		static Vector2 GetNormalizedMousePosition();
		bool GetOrthogonal() const { return m_b_orthogonal_; }

	private:
		SERIALIZER_ACCESS

		Vector3 m_look_at_;
		Quaternion m_mouse_rotation_;

		Vector2 m_previous_mouse_position_;
		Vector2 m_current_mouse_position_;

		Vector3 m_offset_;
		ActorID m_bound_object_id_;

		bool m_b_orthogonal_;

		// Non-serialized
		Matrix m_mouse_rotation_matrix_;
		Matrix m_view_matrix_;
		VPBuffer m_vp_buffer_;
		WeakObject m_bound_object_;
	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Objects::Camera)