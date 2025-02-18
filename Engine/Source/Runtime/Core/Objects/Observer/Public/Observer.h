#pragma once
#include "ObjectBase/Public/ObjectBase.h"

#include "Observer.generated.h"

namespace Engine::Objects
{
	ECLASS(object, serialize)
	class ENGINE_CORE_API Observer : public Abstracts::ObjectBase
	{
		GENERATE_BODY
	public:
		OBJECT_T(DEF_OBJ_T_OBSERVER)

		Observer();
		void Initialize() override;
		~Observer() override;
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;
		void PostRender(const float dt) override;
		void FixedUpdate(const float dt) override;
	};
} // namespace Engine::Objects
