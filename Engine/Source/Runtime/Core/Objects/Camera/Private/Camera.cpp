#include "../Public/Camera.h"
#include "Source/Runtime/Core/Components/Transform/Public/Transform.h"

namespace Engine::Objects
{
	OBJ_CLONE_IMPL(Camera)

	Matrix Camera::GetViewMatrix() const
	{
		return m_view_matrix_;
	}

	Matrix Camera::GetProjectionMatrix() const
	{
		return m_projection_matrix_;
	}

	Matrix Camera::GetWorldMatrix() const
	{
		return m_world_matrix_;
	}

	void Camera::Initialize()
	{
		ObjectBase::Initialize();

		AddComponent<Components::Transform>();
	}

	void Camera::PreUpdate(const float& dt)
	{
		ObjectBase::PreUpdate(dt);
	}

	void Camera::Update(const float& dt)
	{
		ObjectBase::Update(dt);
	}

	void Camera::PreRender(const float& dt)
	{
		ObjectBase::PreRender(dt);
	}

	void Camera::Render(const float& dt)
	{
		ObjectBase::Render(dt);
	}

	void Camera::PostRender(const float& dt)
	{
		ObjectBase::PostRender(dt);
	}

	void Camera::FixedUpdate(const float& dt)
	{
		ObjectBase::FixedUpdate(dt);
	}

	void Camera::SetOrthogonal(bool bOrthogonal)
	{
		m_b_orthogonal_ = bOrthogonal;
	}

	void Camera::SetFixedUp(bool bFixedUp)
	{
		m_b_fixed_up_ = bFixedUp;
	}

	void Camera::SetFOV(float zoom)
	{
		m_fov_ = zoom;
	}

	bool Camera::GetOrthogonal() const
	{
		return m_b_orthogonal_;
	}

	float Camera::GetFOV() const
	{
		return m_fov_;
	}

	void Camera::OnDeserialized()
	{
		ObjectBase::OnDeserialized();
	}
} // namespace Engine::Objects
