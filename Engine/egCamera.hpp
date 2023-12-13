#pragma once
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
		void SetLookAtRotation(Quaternion rotation) { m_look_at_rotation_ = rotation; }

		Quaternion GetRotation();
		Vector3 GetPosition();
		Quaternion GetLookAtRotation() { return m_look_at_rotation_; }

		Matrix GetViewMatrix() const { return m_view_matrix_; }
		Matrix GetProjectionMatrix() const { return m_projection_matrix_; }
		Matrix GetWorldMatrix() const { return m_world_matrix_; }
		Vector3 GetLookAt();

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void OnDeserialized() override;
		void OnImGui() override;

		void BindObject(const WeakObject& object);
		void SetOffset(Vector3 offset);
		void SetOrthogonal(bool bOrthogonal) { m_b_orthogonal_ = bOrthogonal; }

		Vector2 GetWorldMousePosition();
		bool GetOrthogonal() const { return m_b_orthogonal_; }

	private:
		SERIALIZER_ACCESS

		Vector3 m_look_at_;
		Quaternion m_look_at_rotation_;

		Vector3 m_offset_;
		ActorID m_bound_object_id_;

		bool m_b_orthogonal_;

		// Non-serialized
		Matrix m_world_matrix_;
		Matrix m_view_matrix_;
		Matrix m_projection_matrix_;

		PerspectiveBuffer m_wvp_buffer_;
		WeakObject m_bound_object_;
	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Objects::Camera)