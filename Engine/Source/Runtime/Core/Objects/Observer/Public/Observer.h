#pragma once
#include "ObjectBase/Public/ObjectBase.hpp"

namespace Engine::Objects
{
	class ENGINE_CORE_API Observer : public Abstracts::ObjectBase
	{
	public:
		INLINE_COMPILE_TIME_TYPENAME(Observer)
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

	private:
		OBJ_CLONE_DECL
	};
} // namespace Engine::Objects
