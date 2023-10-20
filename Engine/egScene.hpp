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

		template <typename T>
		void AddGameObject(const StrongObject& obj, eLayerType layer)
		{
			m_layers[layer]->AddGameObject<T>(obj);
		}

		template <typename T>
		void RemoveGameObject(const uint64_t id, eLayerType layer)
		{
			m_layers[layer]->RemoveGameObject<T>(id);
		}

		std::vector<WeakObject> GetGameObjects(eLayerType layer)
		{
			return m_layers[layer]->GetGameObjects();
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
		AddGameObject<Objects::Camera>(camera, LAYER_CAMERA);

		const auto light1 = Instantiate<Objects::Light>();
		AddGameObject<Objects::Light>(light1, LAYER_LIGHT);

		const auto light2 = Instantiate<Objects::Light>();
		light2->GetComponent<Component::Transform>().lock()->SetPosition(Vector3(-1.0f, 0.0f, 0.0f));
		light2->SetColor(Vector4(0.0f, 0.0f, 1.0f, 1.0f));
		AddGameObject<Objects::Light>(light2, LAYER_LIGHT);
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
		Graphic::RenderPipeline::BindLightBuffers();

		for (const auto& val : m_layers | std::views::values)
		{
			val->Render();
		}
	}
}
