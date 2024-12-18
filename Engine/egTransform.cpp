#include "pch.h"
#include "egTransform.h"

#include "egImGuiHeler.hpp"
#include "egManagerHelper.hpp"
#include "egRigidbody.h"

SERIALIZE_IMPL
(
	Engine::Components::Transform,
	_ARTAG(_BSTSUPER(Component))
	_ARTAG(m_b_s_absolute_)
	_ARTAG(m_b_r_absolute_)
	_ARTAG(m_previous_position_)
	_ARTAG(m_position_)
	_ARTAG(m_rotation_)
	_ARTAG(m_scale_)
	_ARTAG(m_animation_position_)
	_ARTAG(m_animation_rotation_)
	_ARTAG(m_animation_scale_)
	_ARTAG(m_animation_matrix_)
)

namespace Engine::Components
{
	COMP_CLONE_IMPL(Transform)

	Transform::Transform(const WeakObjectBase& owner)
		: Component(COM_T_TRANSFORM, owner),
		  m_b_s_absolute_(true),
		  m_b_r_absolute_(false),
		  m_previous_position_(Vector3::Zero),
		  m_position_(Vector3::Zero),
		  m_rotation_(Quaternion::Identity),
		  m_scale_(Vector3::One),
		  m_animation_position_(Vector3::Zero),
		  m_animation_rotation_(Quaternion::Identity),
		  m_animation_scale_(Vector3::One),
		  m_animation_matrix_(Matrix::Identity) {}

	void __vectorcall Transform::SetWorldPosition(const Vector3& position)
	{
		Matrix       world     = GetWorldMatrix();
		const Matrix inv_local = GetLocalMatrix().Invert();
		world                  = inv_local * world;
		world                  = world.Invert();

		m_position_ = Vector3::Transform(position, world);
	}

	void __vectorcall Transform::SetWorldRotation(const Quaternion& rotation)
	{
		Quaternion world = GetWorldRotation();
		Quaternion inv_local;
		GetLocalRotation().Inverse(inv_local);
		world = inv_local * world;
		world.Inverse(world);

		m_rotation_ = rotation * world;
	}

	void Transform::SetWorldScale(const Vector3& scale)
	{
		Vector3       world = GetWorldScale();
		const Vector3 local = GetLocalScale();
		world               = world / local;

		m_scale_ = scale / world;
	}

	void __vectorcall Transform::SetLocalPosition(const Vector3& position)
	{
		m_position_ = position;
	}

	void __vectorcall Transform::SetLocalRotation(const Quaternion& rotation)
	{
		m_rotation_ = rotation;
	}

	void __vectorcall Transform::SetLocalScale(const Vector3& scale)
	{
		m_scale_ = scale;
	}

	void Transform::SetLocalMatrix(const Matrix& matrix)
	{
		if (!const_cast<Matrix*>(&matrix)->Decompose(m_scale_, m_rotation_, m_position_))
		{
			throw std::runtime_error
					("Matrix decomposition failed");
		}
	}

	void Transform::SetSizeAbsolute(bool absolute)
	{
		m_b_s_absolute_ = absolute;
	}

	void Transform::SetRotateAbsolute(bool absolute)
	{
		m_b_r_absolute_ = absolute;
	}

	void __vectorcall Transform::SetAnimationPosition(const Vector3& position)
	{
		// SRT * T^-1 = SR
		m_animation_matrix_   = m_animation_matrix_ * Matrix::CreateTranslation(m_animation_position_).Invert();
		m_animation_position_ = position;
		m_animation_matrix_   = m_animation_matrix_ * Matrix::CreateTranslation(m_animation_position_);
	}

	void __vectorcall Transform::SetAnimationRotation(const Quaternion& rotation)
	{
		// rebuild animation matrix
		m_animation_rotation_ = rotation;
		m_animation_matrix_   = Matrix::CreateScale(m_animation_scale_) *
		                        Matrix::CreateFromQuaternion(m_animation_rotation_) *
		                        Matrix::CreateTranslation(m_animation_position_);
	}

	void __vectorcall Transform::SetAnimationScale(const Vector3& scale)
	{
		// S^-1 * SRT = RT
		m_animation_matrix_ = Matrix::CreateScale(m_animation_scale_).Invert() * m_animation_matrix_;
		m_animation_scale_  = scale;
		m_animation_matrix_ = Matrix::CreateScale(m_animation_scale_) * m_animation_matrix_;
	}

	void Transform::SetAnimationMatrix(const Matrix& matrix)
	{
		m_animation_matrix_ = matrix;
	}

	Vector3 Transform::GetWorldPosition()
	{
		Matrix world = GetWorldMatrix();
		return {world._41, world._42, world._43};
	}

	Quaternion Transform::GetWorldRotation() const
	{
		/*if (!m_b_lazy_)
		{
		    const auto   scale = GetWorldScale();
		    const Matrix rot
		    {
		        {m_world_matrix_._11 / scale.x, m_world_matrix_._12 / scale.x, m_world_matrix_._13 / scale.x, 0.f},
		        {m_world_matrix_._21 / scale.y, m_world_matrix_._22 / scale.y, m_world_matrix_._23 / scale.y, 0.f},
		        {m_world_matrix_._31 / scale.z, m_world_matrix_._32 / scale.z, m_world_matrix_._33 / scale.z, 0.f},
		        {0.f, 0.f, 0.f, 1.f}
		    };
	
		    return Quaternion::CreateFromRotationMatrix(rot);
		}*/

		Quaternion    world = GetLocalRotation();
		auto          tr_c  = const_cast<Transform*>(this);
		WeakTransform tr_p  = FindNextTransform(*this);

		while (const auto parent = tr_p.lock())
		{
			if (!tr_c->m_b_r_absolute_)
			{
				world *= parent->GetLocalRotation();
			}

			tr_c = parent.get();
			tr_p = FindNextTransform(*parent);
		}

		return world;
	}

	Vector3 Transform::GetLocalPosition() const
	{
		return m_position_ + GetAnimationPosition();
	}

	Quaternion Transform::GetLocalRotation() const
	{
		return m_rotation_ * GetAnimationRotation();
	}

	Vector3 Transform::GetLocalScale() const
	{
		return m_scale_ * GetAnimationScale();
	}

	Vector3 Transform::GetAnimationPosition() const
	{
		return m_animation_position_;
	}

	Vector3 Transform::GetAnimationScale() const
	{
		return m_animation_scale_;
	}

	Quaternion Transform::GetAnimationRotation() const
	{
		return m_animation_rotation_;
	}

	Vector3 Transform::GetWorldScale() const
	{
		/*if (!m_b_lazy_)
		{
		    const Vector3 x = {m_world_matrix_._11, m_world_matrix_._12, m_world_matrix_._13};
		    const Vector3 y = {m_world_matrix_._21, m_world_matrix_._22, m_world_matrix_._23};
		    const Vector3 z = {m_world_matrix_._31, m_world_matrix_._32, m_world_matrix_._33};
	
		    return {x.Length(), y.Length(), z.Length()};
		}*/

		Vector3       world = GetLocalScale();
		auto          tr_c  = const_cast<Transform*>(this);
		WeakTransform tr_p  = FindNextTransform(*this);

		while (const auto parent = tr_p.lock())
		{
			if (!tr_c->m_b_s_absolute_)
			{
				world *= parent->GetLocalScale();
			}

			tr_c = parent.get();
			tr_p = FindNextTransform(*parent);
		}

		return world;
	}

	Vector3 Transform::Forward() const
	{
		return Vector3::Transform(g_forward, GetWorldRotation());
	}

	Vector3 Transform::Right() const
	{
		return Vector3::Transform(Vector3::Right, GetWorldRotation());
	}

	Vector3 Transform::Up() const
	{
		return Vector3::Transform(Vector3::Up, GetWorldRotation());
	}

	void Transform::Translate(Vector3 translation)
	{
		m_position_ += translation;
	}

	void Transform::Initialize()
	{
		Component::Initialize();
		m_world_previous_position_ = GetWorldPosition();
		m_previous_position_       = GetLocalPosition();
	}

	void Transform::PreUpdate(const float& dt) {}

	void Transform::Update(const float& dt) {}

	void Transform::PostUpdate(const float& dt)
	{
		Component::PostUpdate(dt);

		m_world_previous_position_ = GetWorldPosition();
		m_previous_position_       = GetLocalPosition();
	}

	void Transform::FixedUpdate(const float& dt) {}

	void Transform::OnSerialized()
	{
		Component::OnSerialized();
	}

	void Transform::OnImGui()
	{
		Component::OnImGui();
		ImGui::Indent(2);
		if (ImGuiVector3Editable("Previous Position", GetID(), "previous_position", m_previous_position_))
		{
			GetTaskScheduler().AddTask(
				TASK_TF_UPDATE,
				{ m_previous_position_, GetSharedPtr<Transform>() },
				[](const std::vector<std::any>& params, const float)
				{
					const Vector3 position = std::any_cast<Vector3>(params[0]);
					const boost::shared_ptr<Transform> transform = std::any_cast<boost::shared_ptr<Transform>>(params[1]);

					transform->m_previous_position_ = position;

					if (const Strong<Abstract::ObjectBase>& owner = transform->GetOwner().lock())
					{
						if (const Strong<Rigidbody> rb = owner->GetComponent<Rigidbody>().lock())
						{
							rb->Synchronize();
						}
					}
				}
			);
		}

		if (ImGuiVector3Editable("Position", GetID(), "position", m_position_))
		{
			GetTaskScheduler().AddTask(
				TASK_TF_UPDATE,
				{ m_position_, GetSharedPtr<Transform>() },
				[](const std::vector<std::any>& params, const float)
				{
					const Vector3 position = std::any_cast<Vector3>(params[0]);
					const boost::shared_ptr<Transform> transform = std::any_cast<boost::shared_ptr<Transform>>(params[1]);

					transform->m_position_ = position;

					if (const Strong<Abstract::ObjectBase>& owner = transform->GetOwner().lock())
					{
						if (const Strong<Rigidbody> rb = owner->GetComponent<Rigidbody>().lock())
						{
							rb->Synchronize();
						}
					}
				}
			);
		}

		if (ImGuiVector3Editable("Scale", GetID(), "scale", m_scale_, 0.1f, 0.1f))
		{
			ZeroToEpsilon(m_scale_);
		}

		Vector3 euler = m_rotation_.ToEuler();
		euler         = Vector3
		{
			DirectX::XMConvertToDegrees(euler.x),
			DirectX::XMConvertToDegrees(euler.y),
			DirectX::XMConvertToDegrees(euler.z)
		};

		if (ImGuiVector3Editable("Rotation", GetID(), "rotation", euler, 0.1f, 0.1f))
		{
			m_rotation_ = Quaternion::CreateFromYawPitchRoll
					(
					 DirectX::XMConvertToRadians(euler.y), DirectX::XMConvertToRadians
					 (euler.x), DirectX::XMConvertToRadians(euler.z)
					);
		}

		CheckboxAligned("Size Absolute", m_b_s_absolute_);
		CheckboxAligned("Rotate Absolute", m_b_r_absolute_);
		ImGui::Unindent(2);
	}

	void Transform::OnDeserialized()
	{
		Component::OnDeserialized();
	}

	Matrix Transform::GetLocalMatrix() const
	{
		return Matrix::CreateScale(m_scale_) *
		       Matrix::CreateFromQuaternion(m_rotation_) *
		       Matrix::CreateTranslation(m_position_) *
		       m_animation_matrix_;
	}

	Vector3 Transform::GetWorldPreviousPositionPerFrame() const
	{
		return m_world_previous_position_;
	}

	Vector3 Transform::GetLocalPreviousPositionPerFrame() const
	{
		return m_previous_position_;
	}

	Matrix Transform::GetWorldMatrix() const
	{
		Matrix        world = GetLocalMatrix();
		auto          tr_c  = this;
		WeakTransform tr_p  = FindNextTransform(*this);

		while (const auto parent = tr_p.lock())
		{
			Matrix inv = Matrix::Identity;

			if (tr_c->m_b_s_absolute_)
			{
				inv *= Matrix::CreateScale(parent->GetLocalScale()).Invert();
			}
			if (tr_c->m_b_r_absolute_)
			{
				inv *= Matrix::CreateFromQuaternion(parent->GetLocalRotation()).Invert();
			}

			world *= inv * parent->GetLocalMatrix();

			tr_c = parent.get();
			tr_p = FindNextTransform(*parent);
		}

		return world;
	}

	Transform::Transform()
		: Component(COM_T_TRANSFORM, {}),
		  m_b_s_absolute_(true),
		  m_b_r_absolute_(false),
		  m_previous_position_(Vector3::Zero),
		  m_position_(Vector3::Zero),
		  m_rotation_(Quaternion::Identity),
		  m_scale_(Vector3::One),
		  m_animation_position_(Vector3::Zero),
		  m_animation_rotation_(Quaternion::Identity),
		  m_animation_scale_(Vector3::One),
		  m_animation_matrix_(Matrix::Identity) {}

	WeakTransform Transform::FindNextTransform(const Transform& transform_)
	{
		WeakObjectBase next = transform_.GetOwner().lock()->GetParent();

		while (const auto parent = next.lock())
		{
			if (const auto transform = parent->GetComponent<Transform>().lock())
			{
				return transform;
			}

			next = parent->GetParent();
		}

		return {};
	}
} // namespace Engine::Component
