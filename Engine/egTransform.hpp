#pragma once
#include "egComponent.hpp"
#include <SimpleMath.h>

#include "egD3Device.hpp"

namespace Engine::Component
{
	class Transform : public Abstract::Component
	{
	public:
		Transform() = default;
		Transform(const Transform&) = default;
		~Transform() override = default;

		void SetPosition(const Vector3& position) { m_position_ = position; }
		void SetRotation(const Quaternion& rotation) { m_rotation_ = rotation; }
		void SetScale(const Vector3& scale) { m_scale_ = scale; }

		Vector3 GetPosition() const { return m_position_; }

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

	inline void Transform::Initialize()
	{
	}

	inline void Transform::PreUpdate()
	{
	}

	inline void Transform::Update()
	{
		m_transform_buffer_.scale = Matrix::CreateScale(m_scale_).Transpose();
		m_transform_buffer_.rotation = Matrix::CreateFromQuaternion(m_rotation_).Transpose();
		m_transform_buffer_.translation = Matrix::CreateTranslation(m_position_).Transpose();

		Graphic::RenderPipeline::SetWorldMatrix(m_transform_buffer_);
	}

	inline void Transform::PreRender()
	{
	}

	inline void Transform::Render()
	{
	}
}
