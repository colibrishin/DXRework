#pragma once
#include "egObject.hpp"
#include "egRenderable.hpp"
#include "egCamera.hpp"
#include "egLight.hpp"

namespace Engine
{
	using WeakObject = std::weak_ptr<Abstract::Object>;
	using StrongObject = std::shared_ptr<Abstract::Object>;
	using StrongCamera = std::shared_ptr<Objects::Camera>;
	using StrongLight = std::shared_ptr<Objects::Light>;

	class Scene : public Abstract::Renderable
	{
	public:
		Scene();
		Scene(const Scene& other) = default;
		virtual ~Scene() override = default;

		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Render() override;

		void AddGameObject(const StrongObject& obj)
		{
			m_objects_.emplace(std::reinterpret_pointer_cast<Abstract::Object>(obj));
		}

	private:
		StrongCamera m_camera_;
		StrongLight m_light_;
		std::set<StrongObject> m_objects_;

	};

	inline Scene::Scene()
	{
		m_camera_ = std::make_shared<Objects::Camera>();
		m_camera_->Initialize();

		m_light_ = std::make_shared<Objects::Light>();
		m_light_->Initialize();
	}

	inline void Scene::PreUpdate()
	{
		m_camera_->PreUpdate();
		for (auto& obj : m_objects_)
		{
			obj->PreUpdate();
		}
	}

	inline void Scene::Update()
	{
		m_light_->Update();
		m_camera_->Update();
		for (auto& obj : m_objects_)
		{
			obj->Update();
		}
	}

	inline void Scene::PreRender()
	{
		m_light_->PreRender();
		m_camera_->PreRender();
		for (auto& obj : m_objects_)
		{
			obj->PreRender();
		}
	}

	inline void Scene::Render()
	{
		m_light_->Render();
		m_camera_->Render();
		for (auto& obj : m_objects_)
		{
			obj->Render();
		}
	}
}
