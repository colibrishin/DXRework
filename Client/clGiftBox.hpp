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

		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Load_INTERNAL() override;
		void Initialize() override;
		void FixedUpdate() override;

	private:

	};

	inline GiftBox::GiftBox() : Mesh("./giftbox.obj")
	{
		GiftBox::Initialize();
	}

	inline void GiftBox::PreUpdate()
	{
	}

	inline void GiftBox::Update()
	{
	}

	inline void GiftBox::PreRender()
	{
	}

	inline void GiftBox::Load_INTERNAL()
	{
	}

	inline void GiftBox::Initialize()
	{
		Mesh::Initialize();
	}

	inline void GiftBox::FixedUpdate()
	{
	}
}
