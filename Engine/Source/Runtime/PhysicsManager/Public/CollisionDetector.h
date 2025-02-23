#pragma once
#include <array>
#include <bitset>
#include <set>
#include <mutex>

#include <tbb/concurrent_vector.h>
#include <tbb/concurrent_map.h>

#include "CollisionInfo.h"
#include "Singleton.h"

#include "TypeLibrary.h"
#include "Delegation.hpp"

#include "Scene.h"

#include "CollisionDetector.generated.h"

DEFINE_DELEGATE(OnLayerMaskChange, const Engine::LayerSizeType, const Engine::LayerSizeType);

#ifdef PHYSX_ENABLED
namespace Engine
{
	namespace Physics
	{
		class PhysXSimulationFilterCallback;
	}
}
#endif

namespace Engine::Managers
{
	ECLASS()
	class ENGINE_PHYSICSMANAGER_API CollisionDetector : public Abstracts::Singleton<CollisionDetector>
	{
		GENERATE_BODY
	public:
		DelegateOnLayerMaskChange onLayerMaskChange;

		explicit CollisionDetector(SINGLETON_LOCK_TOKEN) {}

		void Initialize() override;
		void Update(const float dt) override;
		void PreUpdate(const float dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void PostRender(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostUpdate(const float dt) override;

#if WITH_EDITOR
		void OnUIUpdate(UIContext* const parent, const float dt) override;
#endif

		void SetCollisionLayer(LayerSizeType a, LayerSizeType b);
		void UnsetCollisionLayer(LayerSizeType layer, LayerSizeType layer2);
		bool IsCollisionLayer(LayerSizeType layer1, LayerSizeType layer2);

		bool IsCollided(GlobalEntityID id) const;
		bool IsCollided(GlobalEntityID id1, GlobalEntityID id2) const;
		bool IsCollidedInFrame(GlobalEntityID id1, GlobalEntityID id2) const;

		tbb::concurrent_vector<CollisionInfo>& GetCollisionInfo();

	private:
		friend struct SingletonDeleter;
		~CollisionDetector() override;

		void UpdateLayerMask(const Weak<Scene> scene);
		void UpdateScene(const Weak<Scene> scene);

		void TestCollision(const Weak<Abstracts::ObjectBase>& p_lhs, const Weak<Abstracts::ObjectBase>& p_rhs);
		void TestSpeculation(const Weak<Abstracts::ObjectBase>& p_lhs, const Weak<Abstracts::ObjectBase>& p_rhs, float dt);

		void DispatchInactiveExit(const Weak<Abstracts::ObjectBase>& lhs);

		std::mutex m_layer_mask_mutex_;
		bool       m_layer_mask_[RESERVED_LAYER_MAX + CFG_LAYER_COUNT][RESERVED_LAYER_MAX + CFG_LAYER_COUNT]{};

		tbb::concurrent_vector<CollisionInfo> m_collision_produce_queue_;

		tbb::concurrent_map<GlobalEntityID, std::set<GlobalEntityID>>      m_collision_map_;
		tbb::concurrent_map<GlobalEntityID, std::set<GlobalEntityID>> m_frame_collision_map_;

#if WITH_EDITOR
		void UpdateLayerNames(Weak<Scene> scene);

		std::map<std::pair<LayerSizeType, LayerSizeType>, std::string> m_layer_name_storage_;
#endif
		
#ifdef PHYSX_ENABLED
	public:
		uint32_t GetLayerFilter(const eLayerType layer) const;

	private:
		friend class Engine::Physics::PhysXSimulationFilterCallback;
#endif
	};
} // namespace Engine::Managers
