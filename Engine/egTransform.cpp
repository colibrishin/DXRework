#include "pch.hpp"
#include "egTransform.hpp"

#include "egManagerHelper.hpp"

namespace Engine::Component 
{
	Component::Transform::Transform(const std::weak_ptr<Abstract::Object>& owner) : Component(COMPONENT_PRIORITY_TRANSFORM, owner)
	{
	}

	void Transform::Initialize()
	{
	}

	void Transform::PreUpdate()
	{
	}

	void Transform::Update()
	{
	}

	void Transform::PreRender()
	{
	}

	void Transform::Render()
	{
		m_transform_buffer_.scale = Matrix::CreateScale(m_scale_).Transpose();
		m_transform_buffer_.rotation = Matrix::CreateFromQuaternion(m_rotation_).Transpose();
		m_transform_buffer_.translation = Matrix::CreateTranslation(m_position_).Transpose();

		GetRenderPipeline().SetWorldMatrix(m_transform_buffer_);
	}

	void Transform::FixedUpdate()
	{
	}
}
