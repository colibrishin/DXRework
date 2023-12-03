#pragma once

#include "clCharacterController.hpp"
#include "clTriangleMesh.hpp"
#include "../Engine/egCollider.hpp"
#include "../Engine/egManagerHelper.hpp"
#include "../Engine/egTexture.hpp"
#include "../Engine/egObject.hpp"
#include "../Engine/egResourceManager.hpp"
#include "../Engine/egTransform.hpp"
#include "../Engine/egIShader.hpp"
#include "../Engine/egRigidbody.hpp"
#include "../Engine/egNormalMap.hpp"

namespace Client::Object
{
	class TestCube final : public Engine::Abstract::Object
	{
	public:
		TestCube();
		void Initialize() override;
		~TestCube() override;

		inline void PreUpdate(const float& dt) override;
		inline void Update(const float& dt) override;
		inline void PreRender(const float dt) override;
		inline void Render(const float dt) override;
		void FixedUpdate(const float& dt) override;

	private:
		SERIALIZER_ACCESS

	};
}

BOOST_CLASS_EXPORT_KEY(Client::Object::TestCube)
