#pragma once
#include <boost/serialization/assume_abstract.hpp>
#include <boost/smart_ptr/weak_ptr.hpp>

#include "Source/Runtime/Abstracts/CoreRenderable/Public/Renderable.h"
#include "Source/Runtime/Delegation/Public/Delegation.hpp"
#include "Source/Runtime/Serialization/Public/SerializationHelper.hpp"
#include "..\..\..\TypeLibrary\Public\TypeLibrary.h"

DEFINE_DELEGATE(OnLayerChange, const Engine::LayerSizeType);

namespace Engine::Abstracts
{
	class Actor : public Renderable
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
