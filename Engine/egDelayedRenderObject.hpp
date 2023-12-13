#pragma once
#include "egCommon.hpp"
#include "egObject.hpp"

namespace Engine::Objects
{
	class DelayedRenderObject : public Abstract::Object
	{
	public:
		void Initialize() override;
		~DelayedRenderObject() override = default;

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;
		void FixedUpdate(const float& dt) override;

		void OnDeserialized() override;
		void OnImGui() override;

	private:
		SERIALIZER_ACCESS

	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Objects::DelayedRenderObject);