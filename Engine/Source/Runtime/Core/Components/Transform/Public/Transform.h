#pragma once
#include <boost/serialization/export.hpp>
#include "Delegation/Public/Delegation.hpp"
#include "Source/Runtime/Core/Component/Public/Component.h"

#include "Transform.generated.h"

DEFINE_DELEGATE(OnTranfromChanged);

namespace Engine
{
	inline static constexpr Vector3 g_forward = {0, 0, -1.f};
}

namespace Engine::Components
{
	ECLASS()
	class ENGINE_CORE_API Transform final : public Engine::Abstracts::Component
	{
		GENERATE_BODY

	public:
		DelegateOnTranfromChanged onTransformChanged;

		Transform(const Weak<Engine::Abstracts::ObjectBase>& owner);
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
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void PostUpdate(const float dt) override;
		void FixedUpdate(const float dt) override;
		void OnUIUpdate(UIContext* const context, const float dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;
		eComponentUpdatePriority GetUpdatePriority() const override;

		Matrix GetLocalMatrix() const;
		Matrix GetWorldMatrix() const;

	protected:
		Transform();

	private:
		friend class Managers::LerpManager;
		friend class Managers::ShadowManager;
		friend class Managers::Renderer;

		static Weak<Transform> FindNextTransform(const Transform& transform_);

		COMP_CLONE_DECL

		EPROPERTY()
		bool       m_b_s_absolute_;
		EPROPERTY()
		bool       m_b_r_absolute_;
		Vector3    m_previous_position_;
		Vector3    m_world_previous_position_;
		EPROPERTY()
		Vector3    m_position_;
		EPROPERTY()
		Quaternion m_rotation_;
#if WITH_EDITOR
		Vector3    m_euler_rotation_;
#endif
		EPROPERTY()
		Vector3    m_scale_;

		EPROPERTY()
		Vector3    m_animation_position_;
		EPROPERTY()
		Quaternion m_animation_rotation_;
		EPROPERTY()
		Vector3    m_animation_scale_;
		EPROPERTY()
		Matrix     m_animation_matrix_;
	};
} // namespace Engine::Components
