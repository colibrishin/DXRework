#pragma once
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "Source/Runtime/Core/Renderable/Public/Renderable.h"
#include "Source/Runtime/Core/Delegation/Public/Delegation.hpp"
#include "Source/Runtime/Core/Serialization/Public/SerializationHelper.hpp"

DEFINE_DELEGATE(OnLayerChange, const Engine::LayerSizeType);

namespace Engine::Abstracts
{
	class CORE_API Actor : public Renderable
	{
	public:
		DelegateOnLayerChange onLayerChange;
		~Actor() override = default;

		Actor(const Actor& other);

		LayerSizeType   GetLayer() const;
		Weak<Scene>    GetScene() const;
		LocalActorID GetLocalID() const;
		
	protected:
		explicit Actor();

	private:
		SERIALIZE_DECL
		friend class Scene;

		void SetLayer(LayerSizeType layer);
		void SetScene(const Weak<Scene>& scene);
		void SetLocalID(LocalActorID id);

		Weak<Scene>    m_assigned_scene_;
		LayerSizeType   m_layer_;
		LocalActorID m_local_id_;
	};
} // namespace Engine::Abstract

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Engine::Abstract::Actor)
