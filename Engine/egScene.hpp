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

		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Render() override;

		void AddGameObject(const StrongObject& obj, eLayerType layer)
		{
			m_layers[layer]->AddGameObject(obj);
		}

		void RemoveGameObject(const uint64_t id, eLayerType layer)
		{
			m_layers[layer]->RemoveGameObject(id);
		}

	private:
		std::map<eLayerType, StrongLayer> m_layers;

	};

	inline Scene::Scene()
	{
		for(int i = 0; i < LAYER_MAX; ++i)
		{
			m_layers.emplace(static_cast<eLayerType>(i), Instantiate<Layer>(static_cast<eLayerType>(i)));
		}

		const auto camera = Instantiate<Objects::Camera>();
		AddGameObject(camera, LAYER_CAMERA);

		const auto light = Instantiate<Objects::Light>();
		AddGameObject(light, LAYER_LIGHT);
	}

	inline void Scene::PreUpdate()
	{
		for (const auto& val : m_layers | std::views::values)
		{
			val->PreUpdate();
		}
	}

	inline void Scene::Update()
	{
		for (const auto& val : m_layers | std::views::values)
		{
			val->Update();
		}
	}

	inline void Scene::PreRender()
	{
		for (const auto& val : m_layers | std::views::values)
		{
			val->PreRender();
		}
	}

	inline void Scene::Render()
	{
		for (const auto& val : m_layers | std::views::values)
		{
			val->Render();
		}
	}
}
