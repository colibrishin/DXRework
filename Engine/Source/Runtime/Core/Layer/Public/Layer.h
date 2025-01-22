#pragma once
#include "Source/Runtime/Core/Renderable/Public/Renderable.h"
#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/Core/Allocator/Public/Allocator.h"

namespace Engine
{
	class Layer final : public Abstracts::Renderable
	{
	public:
		Layer(const LayerSizeType type);

		~Layer() override = default;

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostUpdate(const float& dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		void                        AddGameObject(const Strong<Abstracts::ObjectBase>& obj);
		void                        RemoveGameObject(GlobalEntityID id);
		Weak<Abstracts::ObjectBase> FindGameObject(GlobalEntityID id) const;
		Weak<Abstracts::ObjectBase> FindGameObjectByLocalID(LocalActorID id) const;
		ConcurrentWeakObjVec        GetGameObjects() const;

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

		SERIALIZE_DECL

		LayerSizeType                                 m_layer_type_;
		aligned_vector<Strong<Abstracts::ObjectBase>> m_objects_;

		// Non-serialized
		ConcurrentWeakObjGlobalMap m_weak_objects_cache_;
	};
} // namespace Engine

BOOST_CLASS_EXPORT_KEY(Engine::Layer)
