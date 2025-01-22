#pragma once
#include "Source/Runtime/CoreEntity/Public/Renderable.h"
#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/Core/Allocator/Public/Allocator.h"

#include "Layer.generated.h"

namespace Engine
{
	ECLASS()
	class ENGINE_CORE_API Layer final : public Abstracts::Renderable
	{
		GENERATE_BODY
	public:
		Layer(const LayerSizeType type);

		~Layer() override = default;

		void Initialize() override;
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void PostRender(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostUpdate(const float dt) override;
		void OnUIUpdate(UIContext* const parent, const float dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		void                        AddGameObject(const Strong<Abstracts::ObjectBase>& obj);
		void                        RemoveGameObject(GlobalEntityID id);
		Weak<Abstracts::ObjectBase> FindGameObject(GlobalEntityID id) const;
		Weak<Abstracts::ObjectBase> FindGameObjectByLocalID(LocalActorID id) const;

		ConcurrentWeakObjVec GetGameObjectsConcurrent() const;
		WeakObjVec           GetGameObjects() const;

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

	private:
		Layer();

		EPROPERTY()
		LayerSizeType                                 m_layer_type_;

		EPROPERTY()
		aligned_vector<Strong<Abstracts::ObjectBase>> m_objects_;

		size_t m_cache_lock_idx_;

		// Non-serialized
#if WITH_EDITOR
		bool m_b_layer_expanded_ = false;
		std::string                m_ui_text_;
#endif
		ConcurrentWeakObjGlobalMap m_concurrent_weak_objects_cache_;
		WeakObjGlobalMap m_weak_objects_cache_;
	};
} // namespace Engine
