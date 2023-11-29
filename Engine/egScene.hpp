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

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void FixedUpdate(const float& dt) override;

		EntityID AddGameObject(const StrongObject& obj, eLayerType layer)
		{
			m_layers[layer]->AddGameObject(obj);
			m_cached_objects_.emplace(obj->GetID(), obj);

			if (const auto tr = obj->GetComponent<Component::Transform>().lock())
			{
				UpdatePosition(obj);
			}

			return obj->GetID();
		}

		void RemoveGameObject(const EntityID id, eLayerType layer)
		{
			const auto obj = m_cached_objects_[id];

			if (const auto locked = obj.lock())
			{
				const auto tr = locked->GetComponent<Component::Transform>().lock();
				bool updated = false;

				if (tr)
				{
					const auto prev_pos = tr->GetPreviousPosition() + Vector3::One * g_octree_negative_round_up;
					const auto pos = tr->GetPosition() + Vector3::One * g_octree_negative_round_up;

					auto& prev_pos_set = m_object_position_tree_(static_cast<int>(prev_pos.x), static_cast<int>(prev_pos.y), static_cast<int>(prev_pos.z));
					auto& pos_set = m_object_position_tree_(static_cast<int>(pos.x), static_cast<int>(pos.y), static_cast<int>(pos.z));

					if (prev_pos_set.contains(obj))
					{
						prev_pos_set.erase(obj);
						updated = true;
					}
					if (pos_set.contains(obj))
					{
						pos_set.erase(obj);
						updated = true;
					}
				}

				if (!updated)
				{
					// @todo: add task for refreshing octree.
				}
			}

			m_cached_objects_.erase(id);
			m_layers[layer]->RemoveGameObject(id);
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
				m_layers[obj->GetLayer()]->RemoveGameObject(id);
				m_layers[type]->AddGameObject(obj);
			}
		}

		auto serialized_layer_begin() noexcept
		{
			return m_layers.begin();
		}

		auto serialized_layer_end() noexcept
		{
			return m_layers.end();
		}

		void UpdatePosition(const WeakObject& obj);
		void GetNearestObjects(const Vector3& pos, std::vector<WeakObject>& out);
		void GetNearbyObjects(const Vector3& pos, const size_t range, std::vector<WeakObject>& out);
		void SearchObjects(const Vector3& pos, const Vector3& dir, std::set<WeakObject, WeakObjComparer>& out, int exhaust = 100);

	private:
		WeakCamera m_mainCamera_;
		std::map<eLayerType, StrongLayer> m_layers;
		std::map<EntityID, WeakObject> m_cached_objects_;
		Octree<std::set<WeakObject, WeakObjComparer>> m_object_position_tree_;

	};

	inline Scene::Scene() : m_object_position_tree_(g_max_map_size, {})
	{
	}

	inline void Scene::PreUpdate(const float& dt)
	{
		m_layers[LAYER_LIGHT]->PreUpdate(dt);

		for (int i = g_early_update_layer_end; i < LAYER_MAX; ++i)
		{
			m_layers[static_cast<eLayerType>(i)]->PreUpdate(dt);
		}
	}

	inline void Scene::Update(const float& dt)
	{
		m_layers[LAYER_LIGHT]->Update(dt);

		for (int i = g_early_update_layer_end; i < LAYER_MAX; ++i)
		{
			m_layers[static_cast<eLayerType>(i)]->Update(dt);
		}
	}

	inline void Scene::PreRender(const float dt)
	{
		m_layers[LAYER_LIGHT]->PreRender(dt);

		for (int i = g_early_update_layer_end; i < LAYER_MAX; ++i)
		{
			m_layers[static_cast<eLayerType>(i)]->PreRender(dt);
		}
	}

	inline void Scene::Render(const float dt)
	{
		GetRenderPipeline().BindLightBuffers();

		m_layers[LAYER_LIGHT]->Render(dt);

		for (int i = g_early_update_layer_end; i < LAYER_MAX; ++i)
		{
			m_layers[static_cast<eLayerType>(i)]->Render(dt);
		}
	}

	inline void Scene::FixedUpdate(const float& dt)
	{
		m_layers[LAYER_LIGHT]->FixedUpdate(dt);

		for (int i = g_early_update_layer_end; i < LAYER_MAX; ++i)
		{
			m_layers[static_cast<eLayerType>(i)]->FixedUpdate(dt);
		}
	}
}
