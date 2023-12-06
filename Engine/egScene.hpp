#pragma once
#include <ranges>

#include "egRenderable.hpp"
#include "../octree/octree.h"
#include <boost/serialization/export.hpp>

namespace Engine
{
	class Scene : public Abstract::Renderable
	{
	public:
		Scene();
		Scene(const Scene& other) = default;
		~Scene() override = default;

		virtual void Initialize_INTERNAL() = 0;
		void Initialize() final;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Save();
		void Render(const float dt) override;
		void FixedUpdate(const float& dt) override;
		void OnDeserialized() override;

		EntityID AddGameObject(const StrongObject& obj, eLayerType layer);
		void RemoveGameObject(const EntityID id, eLayerType layer);

		std::vector<WeakObject> GetGameObjects(eLayerType layer);

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

		auto serialized_layer_begin() noexcept
		{
			return m_layers.begin();
		}

		auto serialized_layer_end() noexcept
		{
			return m_layers.end();
		}

		void AddCacheComponent(const WeakComponent& component);
		void RemoveCacheComponent(const WeakComponent& component);

		template <typename T>
		const std::set<WeakComponent, ComponentPriorityComparer>& GetCachedComponents()
		{
			return m_cached_components_[typeid(T).name()];
		}

		void UpdatePosition(const WeakObject& obj);
		void GetNearestObjects(const Vector3& pos, std::vector<WeakObject>& out);
		void GetNearbyObjects(const Vector3& pos, const size_t range, std::vector<WeakObject>& out);
		void SearchObjects(const Vector3& pos, const Vector3& dir, std::set<WeakObject, WeakObjComparer>& out, int exhaust = 100);

	private:
		SERIALIZER_ACCESS

		void Synchronize(const StrongScene& scene);
		void OpenLoadPopup(bool& is_load_open);

		virtual void AddCustomObject();

		ActorID m_main_camera_local_id_;
		std::map<eLayerType, StrongLayer> m_layers;

		// Non-serialized
		WeakObject m_observer_;
		WeakCamera m_mainCamera_;
		std::set<ActorID> m_assigned_actor_ids_;
		std::map<EntityID, WeakObject> m_cached_objects_;
		std::map<const std::string, std::set<WeakComponent, ComponentPriorityComparer>> m_cached_components_;
		Octree<std::set<WeakObject, WeakObjComparer>> m_object_position_tree_;

	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Scene)