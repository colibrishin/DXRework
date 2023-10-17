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

		void SetPosition(const DirectX::SimpleMath::Vector3& position) { m_position_ = position; }
		void SetRotation(const DirectX::SimpleMath::Quaternion& rotation) { m_rotation_ = rotation; }
		void SetScale(const DirectX::SimpleMath::Vector3& scale) { m_scale_ = scale; }

		Vector3 GetPosition() const { return m_position_; }

		void Initialize() override;
		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Render() override;

	private:
		DirectX::SimpleMath::Vector3 m_position_;
		DirectX::SimpleMath::Quaternion m_rotation_;
		DirectX::SimpleMath::Vector3 m_scale_;
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
		m_transform_buffer_.world = Matrix::CreateScale(m_scale_) * Matrix::CreateFromQuaternion(m_rotation_) * Matrix::CreateTranslation(m_position_);

		Graphic::RenderPipeline::SetWorldMatrix(m_transform_buffer_);
	}

	inline void Transform::PreRender()
	{
	}

	inline void Transform::Render()
	{
	}
}
