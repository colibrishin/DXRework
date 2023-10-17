#pragma once
#include "egComponent.hpp"
#include "egMesh.hpp"
#include "egRenderPipeline.hpp"

namespace Engine::Component
{
	using WeakMesh = std::weak_ptr<Abstract::Mesh>;
	using StrongMesh = std::shared_ptr<Abstract::Mesh>;

	class MeshRenderer : public Abstract::Component
	{
	public:
		void Initialize() override;
		void PreUpdate() override;
		void Update() override;
		void PreRender() override;
		void Render() override;

		void SetMesh(const StrongMesh& mesh) { m_mesh_ = mesh; }

	private:
		StrongMesh m_mesh_;
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

		if(const auto ptr = m_mesh_)
		{
			ptr->Render();
			Graphic::RenderPipeline::DrawIndexed(ptr->GetIndexCount());
		}
	}
}
