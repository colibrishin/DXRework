#pragma once
#include "egCommon.hpp"
#include "egRenderable.h"
#include "egDelegate.hpp"

DEFINE_DELEGATE(OnLayerChange, const Engine::eLayerType);

namespace Engine::Abstract
{
	class Actor : public Renderable
	{
	public:
		DelegateOnLayerChange onLayerChange;

		~Actor() override = default;

		Actor(const Actor& other);

		eLayerType   GetLayer() const;
		WeakScene    GetScene() const;
		LocalActorID GetLocalID() const;

		void OnImGui() override;

	protected:
		explicit Actor();

	private:
		SERIALIZE_DECL
		friend class Scene;

		void SetLayer(eLayerType layer);
		void SetScene(const WeakScene& scene);
		void SetLocalID(LocalActorID id);

		WeakScene    m_assigned_scene_;
		eLayerType   m_layer_;
		LocalActorID m_local_id_;
	};
} // namespace Engine::Abstract

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Engine::Abstract::Actor)
