#pragma once
#include "../Engine/egMesh.hpp"

#include "GeometricPrimitive.h"

namespace Client::Mesh
{
	class GiftBox final : public Engine::Resources::Mesh
	{
	public:
		GiftBox();
		~GiftBox() override = default;

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float& dt) override;
		void Load_CUSTOM() override;
		void Initialize() override;
		void FixedUpdate(const float& dt) override;

	private:

	};

	inline GiftBox::GiftBox() : Mesh("./giftbox.obj")
	{
		GiftBox::Initialize();
	}

	inline void GiftBox::PreUpdate(const float& dt)
	{
	}

	inline void GiftBox::Update(const float& dt)
	{
	}

	inline void GiftBox::PreRender(const float& dt)
	{
	}

	inline void GiftBox::Load_CUSTOM()
	{
	}

	inline void GiftBox::Initialize()
	{
		Mesh::Initialize();
	}

	inline void GiftBox::FixedUpdate(const float& dt)
	{
	}
}
