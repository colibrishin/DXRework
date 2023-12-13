#include "pch.hpp"
#include "egTransform.hpp"

#include "egManagerHelper.hpp"

SERIALIZER_ACCESS_IMPL(Engine::Component::Transform,
	_ARTAG(_BSTSUPER(Component))
	_ARTAG(m_previous_position_)
	_ARTAG(m_position_)
	_ARTAG(m_rotation_)
	_ARTAG(m_scale_)
)

namespace Engine::Component 
{
	Component::Transform::Transform(const WeakObject& owner) : Component(COMPONENT_PRIORITY_TRANSFORM, owner), m_previous_position_(Vector3::Zero), m_position_(Vector3::Zero), m_rotation_(Quaternion::Identity), m_scale_(Vector3::One)
	{
	}

	void Transform::Translate(Vector3 translation)
	{
		m_position_ += translation;
	}

	void Transform::Initialize()
	{
		Component::Initialize();
		m_previous_position_ = m_position_;
	}

	void Transform::PreUpdate(const float& dt)
	{
		m_rotation_ = Quaternion::CreateFromYawPitchRoll(
			DirectX::XMConvertToRadians(m_yaw_pitch_roll_degree_.x), 
			DirectX::XMConvertToRadians(m_yaw_pitch_roll_degree_.y), 
			DirectX::XMConvertToRadians(m_yaw_pitch_roll_degree_.z));
	}

	void Transform::Update(const float& dt)
	{
	}

	void Transform::PreRender(const float& dt)
	{
	}

	void Transform::PostRender(const float& dt)
	{
	}

	void Transform::Render(const float& dt)
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

	void Transform::OnImGui()
	{
		Component::OnImGui();
		ImGui::Indent(2);

		ImGui::Text("Previous Position");
		ImGuiVector3Editable(GetID(), "previous_position", m_previous_position_);

		ImGui::Text("Position");
		ImGuiVector3Editable(GetID(), "position", m_position_);

		ImGui::Text("Rotation");
		ImGuiVector3Editable(GetID(), "yaw_pitch_roll", m_yaw_pitch_roll_degree_);

		ImGui::Text("Scale");
		ImGuiVector3Editable(GetID(), "scale", m_scale_);
		ImGui::Unindent(2);
	}

	void Transform::OnDeserialized()
	{
		Component::OnDeserialized();
		const auto euler = m_rotation_.ToEuler();

		m_yaw_pitch_roll_degree_ = Vector3(
			DirectX::XMConvertToDegrees(euler.y),
			DirectX::XMConvertToDegrees(euler.x),
			DirectX::XMConvertToDegrees(euler.z));
	}

	TypeName Transform::GetVirtualTypeName() const
	{
		return typeid(Transform).name();
	}

	Transform::Transform(): Component(COMPONENT_PRIORITY_TRANSFORM, {}), m_previous_position_(Vector3::Zero), m_position_(Vector3::Zero), m_rotation_(Quaternion::Identity), m_scale_(Vector3::One)
	{
	}
}
