#include "pch.hpp"
#include "egMouseManager.hpp"
#include "egManagerHelper.hpp"

void Engine::Manager::MouseManager::Initialize()
{
	m_current_mouse_position_ = GetNormalizedMousePosition();
	m_previous_mouse_position_ = m_current_mouse_position_;
}

void Engine::Manager::MouseManager::PreUpdate(const float& dt)
{
}

void Engine::Manager::MouseManager::Update(const float& dt)
{
}

void Engine::Manager::MouseManager::FixedUpdate(const float& dt)
{
}

void Engine::Manager::MouseManager::PreRender(const float& dt)
{
	m_current_mouse_position_ = GetNormalizedMousePosition();
	Vector2 delta;
	(m_current_mouse_position_ - m_previous_mouse_position_).Normalize(delta);

	const auto lookRotation = Quaternion::CreateFromYawPitchRoll(delta.x * dt, delta.y * dt, 0.f);
	m_mouse_rotation_ = Quaternion::Concatenate(m_mouse_rotation_, lookRotation);
	m_mouse_rotation_matrix_ = Matrix::CreateFromQuaternion(m_mouse_rotation_);
}

void Engine::Manager::MouseManager::Render(const float& dt)
{
	m_previous_mouse_position_ = m_current_mouse_position_;
}

Vector2 Engine::Manager::MouseManager::GetNormalizedMousePosition()
{
	const Vector2 actual_mouse_position
	{
		static_cast<float>(Engine::GetApplication().GetMouseState().x),
		static_cast<float>(Engine::GetApplication().GetMouseState().y)
	};

	const float x = (((2.0f * actual_mouse_position.x) / Engine::g_window_width) - 1);
	const float y = -(((2.0f * actual_mouse_position.y) / Engine::g_window_height) - 1);

	return {x, y};
}

 const Quaternion& Engine::Manager::MouseManager::GetMouseRotation() const
{
	return m_mouse_rotation_;
}

const Matrix& Engine::Manager::MouseManager::GetMouseRotationMatrix() const
{
	return m_mouse_rotation_matrix_;
}
