#pragma once
#include <SimpleMath.h>
#include <boost/serialization/export.hpp>
#include "egComponent.h"
#include "egDXCommon.h"

namespace Engine::Components
{
	class Transform final : public Abstract::Component
	{
	public:
		COMPONENT_T(COM_T_TRANSFORM)

		Transform(const WeakObjectBase& owner);
		~Transform() override = default;

		void __vectorcall SetWorldPosition(const Vector3& position);
		void __vectorcall SetWorldRotation(const Quaternion& rotation);
		void __vectorcall SetWorldScale(const Vector3& scale);

		void __vectorcall SetLocalPosition(const Vector3& position);
		void __vectorcall SetLocalRotation(const Quaternion& rotation);
		void __vectorcall SetLocalScale(const Vector3& scale);
		void __vectorcall SetLocalMatrix(const Matrix& matrix);
		void              SetSizeAbsolute(bool absolute);
		void              SetRotateAbsolute(bool absolute);

		void __vectorcall SetAnimationPosition(const Vector3& position);
		void __vectorcall SetAnimationRotation(const Quaternion& rotation);
		void __vectorcall SetAnimationScale(const Vector3& scale);
		void __vectorcall SetAnimationMatrix(const Matrix& matrix);

		Vector3    GetWorldPosition();
		Quaternion GetWorldRotation() const;
		Vector3    GetWorldScale() const;
		Vector3    GetWorldPreviousPositionPerFrame() const;
		Vector3    GetLocalPreviousPositionPerFrame() const;
		Vector3    GetLocalPosition() const;
		Quaternion GetLocalRotation() const;
		Vector3    GetLocalScale() const;
		Vector3    GetAnimationPosition() const;
		Vector3    GetAnimationScale() const;
		Quaternion GetAnimationRotation() const;

		// The direction moving towards to the screen if no rotation applied.
		Vector3 Forward() const;
		Vector3 Right() const;
		Vector3 Up() const;

		void Translate(Vector3 translation);

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PostUpdate(const float& dt) override;
		void FixedUpdate(const float& dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;
		void OnImGui() override;

		Matrix GetLocalMatrix() const;
		Matrix GetWorldMatrix() const;

	protected:
		Transform();

	private:
		friend class Manager::Physics::LerpManager;
		friend class Manager::Graphics::ShadowManager;
		friend class Manager::Graphics::Renderer;

		static WeakTransform FindNextTransform(const Transform& transform_);

		SERIALIZE_DECL
		COMP_CLONE_DECL

		bool       m_b_s_absolute_;
		bool       m_b_r_absolute_;
		Vector3    m_previous_position_;
		Vector3    m_world_previous_position_;
		Vector3    m_position_;
		Quaternion m_rotation_;
		Vector3    m_scale_;

		Vector3    m_animation_position_;
		Quaternion m_animation_rotation_;
		Vector3    m_animation_scale_;
		Matrix     m_animation_matrix_;
	};
} // namespace Engine::Component

BOOST_CLASS_EXPORT_KEY(Engine::Components::Transform)
