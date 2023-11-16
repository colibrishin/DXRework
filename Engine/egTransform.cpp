#include "pch.hpp"
#include "egTransform.hpp"

#include "egManagerHelper.hpp"

namespace Engine::Component 
{
	Component::Transform::Transform(const std::weak_ptr<Abstract::Object>& owner) : Component(COMPONENT_PRIORITY_TRANSFORM, owner)
	{
	}

	void Transform::Translate(Vector3 translation)
	{
		m_position_ += translation;
	}

	void Transform::Initialize()
	{
	}

	void Transform::PreUpdate(const float& dt)
	{
	}

	void Transform::Update(const float& dt)
	{
	}

	void Transform::PreRender(const float dt)
	{
	}

	void Transform::Render(const float dt)
	{
		m_transform_buffer_.scale = Matrix::CreateScale(m_scale_).Transpose();
		m_transform_buffer_.rotation = Matrix::CreateFromQuaternion(m_rotation_).Transpose();
		m_transform_buffer_.translation = Matrix::CreateTranslation(m_position_).Transpose();

		GetRenderPipeline().SetWorldMatrix(m_transform_buffer_);
	}

	void Transform::FixedUpdate(const float& dt)
	{
	}
}
