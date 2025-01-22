#pragma once
#include "Source/Runtime/Core/Delegation/Public/Delegation.hpp"
#include "Source/Runtime/CoreEntity/Public/Renderable.h"
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

DEFINE_DELEGATE(OnLayerChange, const Engine::LayerSizeType);

POLYMORPHIC_TYPE_MAP(Engine::Abstracts::Actor, Engine::Abstracts::Renderable)

namespace Engine::Abstracts
{
	class ENGINE_CORE_API Actor : public Renderable
	{
	public:
		INLINE_COMPILE_TIME_TYPENAME(Actor)

		DelegateOnLayerChange onLayerChange;
		~Actor() override = default;

		Actor(const Actor& other);

		LayerSizeType       GetLayer() const;
		Weak<Scene>         GetScene() const;
		const LocalActorID& GetLocalID() const;

		void OnUIUpdate(UIContext* const parent, const float dt) override;
		
	protected:
		SERIALIZE_DECL
		explicit Actor();

	private:
		friend class Scene;

		void SetLayer(LayerSizeType layer);
		void SetScene(const Weak<Scene>& scene);
		void SetLocalID(LocalActorID id);

		Weak<Scene>   m_assigned_scene_;
		LayerSizeType m_layer_;
		LocalActorID  m_local_id_;
	};
} // namespace Engine::Abstracts

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Engine::Abstracts::Actor)