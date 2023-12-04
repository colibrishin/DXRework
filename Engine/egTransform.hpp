#pragma once
#include "egComponent.hpp"
#include <SimpleMath.h>
#include <boost/serialization/export.hpp>
#include "egDXCommon.h"

namespace Engine::Manager::Physics
{
	class LerpManager;
}

namespace Engine::Component
{
	class Transform : public Abstract::Component
	{
	public:
		Transform(const WeakObject& owner);
		~Transform() override = default;

		void SetPosition(const Vector3& position) { m_position_ = position; }
		void SetRotation(const Quaternion& rotation) { m_rotation_ = rotation; }
		void SetScale(const Vector3& scale) { m_scale_ = scale; }
		
		void Translate(Vector3 translation);

		Vector3 GetPosition() const { return m_position_; }
		Vector3 GetPreviousPosition() const { return m_previous_position_; }
		Quaternion GetRotation() const { return m_rotation_; }
		Vector3 GetScale() const { return m_scale_; }

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void FixedUpdate(const float& dt) override;

		void OnImGui() override;
		void OnDeserialized() override;

	protected:
		Transform();

	private:
		friend class Manager::Physics::LerpManager;
		SERIALIZER_ACCESS

		Vector3 m_previous_position_;
		Vector3 m_position_;
		Quaternion m_rotation_;
		Vector3 m_scale_;

		// Non-serialized
		TransformBuffer m_transform_buffer_;
	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Component::Transform)