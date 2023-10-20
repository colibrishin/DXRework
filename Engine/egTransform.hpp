#pragma once
#include "egComponent.hpp"
#include <SimpleMath.h>

#include "egD3Device.hpp"

namespace Engine::Component
{
	class Transform : public Abstract::Component
	{
	public:
		Transform(const std::weak_ptr<Abstract::Object>& owner);
		Transform(const Transform&) = default;
		~Transform() override = default;

		void SetPosition(const Vector3& position) { m_position_ = position; }
		void SetRotation(const Quaternion& rotation) { m_rotation_ = rotation; }
		void SetScale(const Vector3& scale) { m_scale_ = scale; }

		Vector3 GetPosition() const { return m_position_; }
		Quaternion GetRotation() const { return m_rotation_; }
		Vector3 GetScale() const { return m_scale_; }

		void Initialize() override;
		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Render() override;

	private:
		Vector3 m_position_ = Vector3::Zero;
		Quaternion m_rotation_;
		Vector3 m_scale_ = Vector3::One;
		TransformBuffer m_transform_buffer_;
	};

	inline Transform::Transform(const std::weak_ptr<Abstract::Object>& owner) : Component(owner)
	{
	}

	inline void Transform::Initialize()
	{
	}

	inline void Transform::PreUpdate()
	{
	}

	inline void Transform::Update()
	{
	}

	inline void Transform::PreRender()
	{
	}

	inline void Transform::Render()
	{
		m_transform_buffer_.scale = Matrix::CreateScale(m_scale_).Transpose();
		m_transform_buffer_.rotation = Matrix::CreateFromQuaternion(m_rotation_).Transpose();
		m_transform_buffer_.translation = Matrix::CreateTranslation(m_position_).Transpose();

		Graphic::RenderPipeline::SetWorldMatrix(m_transform_buffer_);
	}
}
