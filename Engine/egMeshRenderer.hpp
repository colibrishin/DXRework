#pragma once
#include "egComponent.hpp"
#include "egMesh.hpp"
#include "egRenderPipeline.hpp"
#include "egTexture.hpp"

namespace Engine::Component
{
	using WeakMesh = std::weak_ptr<Resources::Mesh>;
	using WeakTexture = std::weak_ptr<Resources::Texture>;
	using StrongMesh = std::shared_ptr<Resources::Mesh>;
	using StrongTexture = std::shared_ptr<Resources::Texture>;

	class MeshRenderer : public Abstract::Component
	{
	public:
		void Initialize() override;
		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Render() override;

		void SetMesh(const WeakMesh& mesh) { m_mesh_ = mesh; }
		void SetTexture(const WeakTexture& texture) { m_texture_ = texture; }

	private:
		WeakMesh m_mesh_;
		WeakTexture m_texture_;
	};

	inline void MeshRenderer::Initialize()
	{
	}

	inline void MeshRenderer::PreUpdate()
	{
	}

	inline void MeshRenderer::Update()
	{
	}

	inline void MeshRenderer::PreRender()
	{
	}

	inline void MeshRenderer::Render()
	{
		Graphic::RenderPipeline::SetShader(L"vs_default");
		Graphic::RenderPipeline::SetShader(L"ps_default");

		if(const auto ptr = m_mesh_.lock())
		{
			ptr->Render();

			if(const auto texture = m_texture_.lock())
			{
				texture->Render();
			}

			Graphic::RenderPipeline::DrawIndexed(ptr->GetIndexCount());
		}
	}
}
