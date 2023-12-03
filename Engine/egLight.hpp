#pragma once
#include <bitset>

#include "egCommon.hpp"
#include "egObject.hpp"
#include "egRenderPipeline.hpp"
#include "egTransform.hpp"
#include "egObject.hpp"

namespace Engine::Objects
{
	class Light final : public Abstract::Object
	{
	public:
		Light() : Object(), m_light_id_(0)
		{
		}

		~Light() override;

		void SetColor(Vector4 color);
		void SetPosition(Vector3 position);

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;

		void OnDeserialized() override;

	private:
		SERIALIZER_ACCESS

		UINT m_light_id_;
		Vector4 m_color_;
		inline static std::bitset<g_max_lights> s_light_map_{};
	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Objects::Light)