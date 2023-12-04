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

	void Transform::OnImGui()
	{
		Component::OnImGui();
		ImGui::Indent(2);

		ImGui::Text("Previous Position");
		ImGuiVector3Editable(m_previous_position_);

		ImGui::Text("Position");
		ImGuiVector3Editable(m_position_);

		ImGui::Text("Rotation");
		ImGuiQuaternionEditable(m_rotation_);

		ImGui::Text("Scale");
		ImGuiVector3Editable(m_scale_);
		ImGui::Unindent(2);
	}

	void Transform::OnDeserialized()
	{
		Component::OnDeserialized();
	}

	Transform::Transform(): Component(COMPONENT_PRIORITY_TRANSFORM, {}), m_previous_position_(Vector3::Zero), m_position_(Vector3::Zero), m_rotation_(Quaternion::Identity), m_scale_(Vector3::One)
	{
	}
}
