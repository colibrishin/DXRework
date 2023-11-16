#pragma once
#include "egTexture.hpp"

namespace Engine::Resources
{
	class NormalMap : public Texture
	{
	public:
		explicit NormalMap(std::filesystem::path path) : Texture(std::move(path))
		{}

		~NormalMap() override = default;

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PreRender(const float dt) override;
		void Render(const float dt) override;

		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;
	};

	inline void NormalMap::Initialize()
	{
		Texture::Initialize();
	}

	inline void NormalMap::PreUpdate(const float& dt)
	{
		Texture::PreUpdate(dt);
	}

	inline void NormalMap::Update(const float& dt)
	{
		Texture::Update(dt);
	}

	inline void NormalMap::PreRender(const float dt)
	{
		Texture::PreRender(dt);
	}

	inline void NormalMap::Render(const float dt)
	{
		GetRenderPipeline().BindResource(SR_NORMAL_MAP, m_texture_view_.Get());
	}

	inline void NormalMap::Load_INTERNAL()
	{
		Texture::Load_INTERNAL();
	}

	inline void NormalMap::Unload_INTERNAL()
	{
		Texture::Unload_INTERNAL();
	}
}
