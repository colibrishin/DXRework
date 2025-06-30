#pragma once
#include "Renderable.h"
#include "ConcurrentTypeLibrary.h"
#include "Allocator.h"
#include "SingletonSpinLock.h"

#include "Layer.generated.h"

namespace Engine
{
	ECLASS(serialize)
	class ENGINE_CORE_API Layer final : public Abstracts::Renderable
	{
		GENERATE_BODY
	public:
		Layer(const LayerSizeType type);

		~Layer() override;

		void Initialize() override;
        virtual void BeginPlay( const float dt );
        virtual void EndPlay( const float dt );
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void PostRender(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostUpdate(const float dt) override;

#if WITH_EDITOR
		void OnUIUpdate(UIContext* const parent, const float dt) override;
#endif

		void OnSerialized() override;
		void OnDeserialized() override;

		void                        AddGameObject(const Strong<Abstracts::ObjectBase>& obj);
		void                        RemoveGameObject(GlobalEntityID id);
		Weak<Abstracts::ObjectBase> FindGameObject(GlobalEntityID id) const;
		Weak<Abstracts::ObjectBase> FindGameObjectByLocalID(LocalActorID id) const;

		ConcurrentWeakObjVec GetGameObjectsConcurrent() const;
		WeakObjVec           GetGameObjects() const;
        LayerSizeType        GetLayerType() const;

		void clear();

		auto begin() noexcept;

		auto end() noexcept;

		auto begin() const noexcept;

		auto end() const noexcept;

		auto cbegin() const noexcept;

		auto cend() const noexcept;

		auto size() const noexcept;

		size_t Empty() const noexcept;

	private:
		Layer();

		EPROPERTY()
		LayerSizeType                                 m_layer_type_;

		EPROPERTY()
		aligned_vector<Strong<Abstracts::ObjectBase>> m_objects_;

		SpinLockTicket m_cache_lock_idx_;

		// Non-serialized
		ConcurrentWeakObjGlobalMap m_concurrent_weak_objects_cache_;
		WeakObjGlobalMap m_weak_objects_cache_;
	};
} // namespace Engine
