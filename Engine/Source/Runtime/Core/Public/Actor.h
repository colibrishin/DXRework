#pragma once
#include "Delegation.hpp"
#include "Renderable.h"
#include "TypeLibrary.h"

#include "Actor.generated.h"

DEFINE_DELEGATE(OnLayerChange, const Engine::LayerSizeType);

namespace Engine::Abstracts
{
	ECLASS(abstract, serialize)
	class ENGINE_CORE_API Actor : public Renderable
	{
		GENERATE_BODY

	public:
		DelegateOnLayerChange onLayerChange;
		~Actor() override = default;

		Actor(const Actor& other);

		LayerSizeType       GetLayer() const;
		Weak<Scene>         GetScene() const;
		const LocalActorID& GetLocalID() const;

#if WITH_EDITOR
		void OnUIUpdate(UIContext* const parent, const float dt) override;
#endif

	protected:
		explicit Actor();

	private:
		friend class Scene;

		void SetLayer(LayerSizeType layer);
		void SetScene(const Weak<Scene>& scene);
		void SetLocalID(LocalActorID id);

		EPROPERTY()
		LayerSizeType m_layer_;

		EPROPERTY()
		LocalActorID  m_local_id_;

		Weak<Scene>   m_assigned_scene_;
	};
} // namespace Engine::Abstracts
