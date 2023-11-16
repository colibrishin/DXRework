#pragma once
#include <ranges>

#include "egObject.hpp"
#include "egRenderable.hpp"
#include "egCamera.hpp"
#include "egLayer.hpp"
#include "egLight.hpp"
#include "egHelper.hpp"

namespace Engine
{
	using StrongCamera = std::shared_ptr<Objects::Camera>;
	using StrongLight = std::shared_ptr<Objects::Light>;
	using StrongLayer = std::shared_ptr<Layer>;

	class Scene : public Abstract::Renderable
	{
	public:
		Scene();
		Scene(const Scene& other) = default;
		~Scene() override = default;

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void FixedUpdate(const float& dt) override;

		template <typename T>
		void AddGameObject(const std::shared_ptr<T>& obj, eLayerType layer)
		{
			m_layers[layer]->AddGameObject<T>(obj);
			m_gameObjects_.emplace(obj->GetID(), obj);
		}

		template <typename T>
		void RemoveGameObject(const uint64_t id, eLayerType layer)
		{
			m_layers[layer]->RemoveGameObject<T>(id);
			m_gameObjects_.erase(id);
		}

		std::vector<WeakObject> GetGameObjects(eLayerType layer)
		{
			return m_layers[layer]->GetGameObjects();
		}

		WeakObject FindGameObject(uint64_t id)
		{
			if (m_gameObjects_.contains(id))
			{
				return m_gameObjects_[id];
			}

			return {};
		}

	private:
		std::map<eLayerType, StrongLayer> m_layers;
		std::map<uint64_t, WeakObject> m_gameObjects_;

	};

	inline Scene::Scene()
	{
		for(int i = 0; i < LAYER_MAX; ++i)
		{
			m_layers.emplace(static_cast<eLayerType>(i), Instantiate<Layer>(static_cast<eLayerType>(i)));
		}

		const auto camera = Instantiate<Objects::Camera>();
		AddGameObject(camera, LAYER_CAMERA);

		const auto light1 = Instantiate<Objects::Light>();
		AddGameObject(light1, LAYER_LIGHT);
		light1->SetPosition(Vector3(5.0f, 5.0f, 5.0f));

		const auto light2 = Instantiate<Objects::Light>();
		light2->SetPosition(Vector3(-5.0f, 5.0f, -5.0f));
		light2->SetColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
		AddGameObject(light2, LAYER_LIGHT);
	}

	inline void Scene::PreUpdate(const float& dt)
	{
		for (const auto& val : m_layers | std::views::values)
		{
			val->PreUpdate(dt);
		}
	}

	inline void Scene::Update(const float& dt)
	{
		for (const auto& val : m_layers | std::views::values)
		{
			val->Update(dt);
		}
	}

	inline void Scene::PreRender(const float dt)
	{
		for (const auto& val : m_layers | std::views::values)
		{
			val->PreRender(dt);
		}
	}

	inline void Scene::Render(const float dt)
	{
		GetRenderPipeline().BindLightBuffers();

		for (const auto& val : m_layers | std::views::values)
		{
			val->Render(dt);
		}
	}

	inline void Scene::FixedUpdate(const float& dt)
	{
		for (const auto& val : m_layers | std::views::values)
		{
			val->FixedUpdate(dt);
		}
	}
}
