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
		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Render() override;

		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;
	};

	inline void NormalMap::Initialize()
	{
		Texture::Initialize();
	}

	inline void NormalMap::PreUpdate()
	{
		Texture::PreUpdate();
	}

	inline void NormalMap::Update()
	{
		Texture::Update();
	}

	inline void NormalMap::PreRender()
	{
		Texture::PreRender();
	}

	inline void NormalMap::Render()
	{
		Graphic::RenderPipeline::BindResource(SR_NORMAL_MAP, m_texture_view_.Get());
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
