#include "pch.hpp"
#include "egLight.hpp"
#include "egCamera.hpp"
#include "egManagerHelper.hpp"

SERIALIZER_ACCESS_IMPL(
	Engine::Objects::Light,
	_ARTAG(_BSTSUPER(Object))
	_ARTAG(m_light_id_)
	_ARTAG(m_color_))

namespace Engine::Objects
{
	Light::~Light()
	{
		s_light_map_.reset(m_light_id_);
		GetRenderPipeline().SetLight(m_light_id_, Matrix::Identity, Color{0.0f, 0.0f, 0.0f, 0.0f});
	}

	void Light::SetColor(Vector4 color)
	{
		m_color_ = color;
	}

	void Light::SetPosition(Vector3 position)
	{
		const auto transform = GetComponent<Component::Transform>();
		transform.lock()->SetPosition(position);
	}

	void Light::Initialize()
	{
		assert(s_light_map_.count() < g_max_lights);

		for (int i = 0; i < g_max_lights; ++i)
		{
			if (!s_light_map_[i])
			{
				m_light_id_ = i;
				s_light_map_.set(i);
				break;
			}
		}

		AddComponent<Component::Transform>();
		m_color_ = Vector4{1.0f, 1.0f, 1.0f, 1.0f};
		m_offset_ = Vector3::Down;
		SetCulled(false);
	}

	void Light::PreUpdate(const float& dt)
	{
		Object::PreUpdate(dt);
	}

	void Light::Update(const float& dt)
	{
		Object::Update(dt);
	}

	void Light::PreRender(const float dt)
	{
		Object::PreRender(dt);

		const auto tr = GetComponent<Component::Transform>().lock();

		const auto world = Matrix::Identity * Matrix::CreateFromQuaternion(tr->GetRotation()) * Matrix::CreateTranslation(tr->GetPosition());

		if (const auto scene = GetScene().lock())
		{
			if (const auto camera = scene->GetMainCamera().lock())
			{
				Vector3 light_dir;
				// [end - start]
				(tr->GetPosition()).Normalize(light_dir);

				GetShadowManager().GetCascadeShadow(
					-light_dir, 
					m_shadow_buffer_.cascade_positions,
					m_shadow_buffer_.view,
					m_shadow_buffer_.proj, 
					m_shadow_buffer_.end_clip_spaces);

				// DX11 uses column major matrix
				for (auto& view : m_shadow_buffer_.view)
				{
					view = view.Transpose();
				}

				for (auto& proj : m_shadow_buffer_.proj)
				{
					proj = proj.Transpose();
				}
			}
		}

		GetRenderPipeline().SetLight(m_light_id_, world.Transpose(), m_color_);	
	}

	void Light::Render(const float dt)
	{
		Object::Render(dt);

		const auto tr = GetComponent<Component::Transform>().lock();
		GetRenderPipeline().SetShadow(m_light_id_, m_shadow_buffer_);
#ifdef _DEBUG
		const BoundingSphere sphere (tr->GetPosition(), 0.5f);
		GetDebugger().Draw(sphere, Colors::Yellow);
#endif
	}

	void Light::OnDeserialized()
	{
		Object::OnDeserialized();
	}
}