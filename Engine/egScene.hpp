#pragma once
#include <ranges>

#include "egObject.hpp"
#include "egRenderable.hpp"
#include "egCamera.hpp"
#include "egLayer.hpp"
#include "egLight.hpp"
#include "egHelper.hpp"
#include "../octree/octree.h"

namespace Engine
{
	using StrongCamera = std::shared_ptr<Objects::Camera>;
	using WeakCamera = std::weak_ptr<Objects::Camera>;
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
		EntityID AddGameObject(const std::shared_ptr<T>& obj, eLayerType layer)
		{
			m_layers[layer]->AddGameObject<T>(obj);
			m_cached_objects_.emplace(obj->GetID(), obj);
			if (const auto tr = obj->template GetComponent<Component::Transform>().lock())
			{
				UpdatePosition(obj);
			}

			return obj->GetID();
		}

		template <typename T>
		void RemoveGameObject(const EntityID id, eLayerType layer)
		{
			const auto obj = m_cached_objects_[id];

			if (const auto locked = obj.lock())
			{
				const auto tr = locked->GetComponent<Component::Transform>().lock();
				bool updated = false;

				if (tr)
				{
					const auto prev_pos = tr->GetPreviousPosition();
					const auto pos = tr->GetPosition();

					if (m_object_position_tree_(prev_pos.x, prev_pos.y, prev_pos.z).contains(obj))
					{
						m_object_position_tree_(prev_pos.x, prev_pos.y, prev_pos.z).erase(obj);
						updated = true;
					}
					if (m_object_position_tree_(pos.x, pos.y, pos.z).contains(obj))
					{
						m_object_position_tree_(pos.x, pos.y, pos.z).erase(obj);
						updated = true;
					}
				}

				if (!updated)
				{
					// @todo: add task for refreshing octree.
				}
			}

			m_cached_objects_.erase(id);
			m_layers[layer]->RemoveGameObject<T>(id);
		}

		std::vector<WeakObject> GetGameObjects(eLayerType layer)
		{
			return m_layers[layer]->GetGameObjects();
		}

		WeakObject FindGameObject(EntityID id)
		{
			if (m_cached_objects_.contains(id))
			{
				return m_cached_objects_[id];
			}

			return {};
		}

		WeakCamera GetMainCamera() const
		{
			return m_mainCamera_;
		}

		void ChangeLayer(EntityID id, eLayerType type)
		{
			if (const auto obj = FindGameObject(id).lock())
			{
				m_layers[obj->GetLayer()]->RemoveGameObject<Abstract::Object>(id);
				m_layers[type]->AddGameObject<Abstract::Object>(obj);
			}
		}

		void UpdatePosition(const WeakObject& obj);
		void GetNearestObjects(const Vector3& pos, std::vector<WeakObject>& out);
		void GetNearbyObjects(const Vector3& pos, const size_t range, std::vector<WeakObject>& out);

	private:
		WeakCamera m_mainCamera_;
		std::map<eLayerType, StrongLayer> m_layers;
		std::map<EntityID, WeakObject> m_cached_objects_;
		Octree<std::set<WeakObject, WeakObjComparer>> m_object_position_tree_;

	};

	inline Scene::Scene() : m_object_position_tree_(g_max_map_size, {})
	{
		for(int i = 0; i < LAYER_MAX; ++i)
		{
			m_layers.emplace(static_cast<eLayerType>(i), Instantiate<Layer>(static_cast<eLayerType>(i)));
		}

		const auto camera = Instantiate<Objects::Camera>();
		AddGameObject(camera, LAYER_CAMERA);

		m_mainCamera_ = camera;

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
		m_layers[LAYER_LIGHT]->PreUpdate(dt);
		m_layers[LAYER_CAMERA]->PreUpdate(dt);

		for (int i = g_early_update_layer_end; i < LAYER_MAX; ++i)
		{
			m_layers[static_cast<eLayerType>(i)]->PreUpdate(dt);
		}
	}

	inline void Scene::Update(const float& dt)
	{
		m_layers[LAYER_LIGHT]->Update(dt);
		m_layers[LAYER_CAMERA]->Update(dt);

		for (int i = g_early_update_layer_end; i < LAYER_MAX; ++i)
		{
			m_layers[static_cast<eLayerType>(i)]->Update(dt);
		}
	}

	inline void Scene::PreRender(const float dt)
	{
		m_layers[LAYER_LIGHT]->PreRender(dt);
		m_layers[LAYER_CAMERA]->PreRender(dt);

		for (int i = g_early_update_layer_end; i < LAYER_MAX; ++i)
		{
			m_layers[static_cast<eLayerType>(i)]->PreRender(dt);
		}
	}

	inline void Scene::Render(const float dt)
	{
		GetRenderPipeline().BindLightBuffers();

		m_layers[LAYER_LIGHT]->Render(dt);
		m_layers[LAYER_CAMERA]->Render(dt);

		for (int i = g_early_update_layer_end; i < LAYER_MAX; ++i)
		{
			m_layers[static_cast<eLayerType>(i)]->Render(dt);
		}
	}

	inline void Scene::FixedUpdate(const float& dt)
	{
		m_layers[LAYER_LIGHT]->FixedUpdate(dt);
		m_layers[LAYER_CAMERA]->FixedUpdate(dt);

		for (int i = g_early_update_layer_end; i < LAYER_MAX; ++i)
		{
			m_layers[static_cast<eLayerType>(i)]->FixedUpdate(dt);
		}
	}
}
