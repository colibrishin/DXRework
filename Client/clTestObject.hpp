#pragma once

#include "clTriangleMesh.hpp"
#include "../Engine/egCollider.hpp"
#include "../Engine/egManagerHelper.hpp"
#include "../Engine/egTexture.hpp"
#include "../Engine/egObject.hpp"
#include "../Engine/egResourceManager.hpp"
#include "../Engine/egTransform.hpp"
#include "../Engine/egIShader.hpp"
#include "../Engine/egRigidbody.hpp"

namespace Client::Object
{
	class TestObject : public Engine::Abstract::Object
	{
	public:
		TestObject() : Engine::Abstract::Object() { }
		void Initialize() override;
		~TestObject() override = default;

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Render(const float& dt) override;
		void PostRender(const float& dt) override;

	private:
		SERIALIZER_ACCESS
	};
}

BOOST_CLASS_EXPORT_KEY(Client::Object::TestObject)