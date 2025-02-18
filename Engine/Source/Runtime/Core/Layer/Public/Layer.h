#pragma once
#include "Source/Runtime/CoreEntity/Public/Renderable.h"
#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/Core/Allocator/Public/Allocator.h"
#include "SingletonSpinLock/Public/SingletonSpinLock.h"

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

		void clear();

		auto begin() noexcept
		{
			return m_objects_.begin();
		}

		auto end() noexcept
		{
			return m_objects_.end();
		}

		auto begin() const noexcept
		{
			return m_objects_.begin();
		}

		auto end() const noexcept
		{
			return m_objects_.end();
		}

		auto cbegin() const noexcept
		{
			return m_objects_.cbegin();
		}

		auto cend() const noexcept
		{
			return m_objects_.cend();
		}

		auto size() const noexcept
		{
			return m_objects_.size();
		}

		size_t Empty() const noexcept 
		{
			return m_objects_.empty();
		}

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
