#include "pch.hpp"
#include "egTransform.hpp"

#include "egManagerHelper.hpp"

namespace Engine::Component 
{
	Component::Transform::Transform(const std::weak_ptr<Abstract::Object>& owner) : Component(COMPONENT_PRIORITY_TRANSFORM, owner), m_position_(Vector3::Zero), m_previous_position_(Vector3::Zero), m_scale_(Vector3::One), m_rotation_(Quaternion::Identity)
	{
	}

	void Transform::Translate(Vector3 translation)
	{
		m_position_ += translation;
	}

	void Transform::Initialize_INTERNAL()
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
		m_previous_position_ = m_position_;
	}

	void Transform::FixedUpdate(const float& dt)
	{
	}

	void Transform::OnLayerChanging()
	{
	}

	void Transform::OnLayerChanged()
	{
	}

	void Transform::OnCreate()
	{
		if (const auto scene = GetOwner().lock()->GetScene().lock())
		{
			scene->AddComponent<Transform>(GetSharedPtr<Transform>());
		}
	}

	void Transform::OnDestroy()
	{
		if (const auto scene = GetOwner().lock()->GetScene().lock())
		{
			scene->RemoveComponent<Transform>(GetSharedPtr<Transform>());
		}
	}

	void Transform::OnSceneChanging()
	{
		OnDestroy();
	}

	void Transform::OnSceneChanged()
	{
		OnCreate();
	}
}
