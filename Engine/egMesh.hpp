#pragma once
#include <d3d11.h>
#include <filesystem>
#include <vector>
#include <windows.h>
#include <SimpleMath.h>
#include <wrl/client.h>

#include "egCommon.hpp"
#include "egD3Device.hpp"
#include "egRenderPipeline.hpp"
#include "egRenderable.hpp"


namespace Engine::Abstract
{
	using namespace DirectX::SimpleMath;
	using Microsoft::WRL::ComPtr;

	class Mesh : public Renderable
	{
	public:
		virtual ~Mesh() override = default;
		void Initialize() override;

		virtual void Load() = 0;
		virtual void SetPath(const std::filesystem::path& path) { m_path_ = path; }

		UINT GetIndexCount() const { return m_indices_.size(); }

	protected:
		Mesh() = default;

	public:
		void Update() override;
		void Render() override;

	protected:
		std::vector<VertexElement> m_vertices_;
		std::vector<UINT> m_indices_;

		ComPtr<ID3D11Buffer> m_vertex_buffer_;
		ComPtr<ID3D11Buffer> m_index_buffer_;

	private:
		std::filesystem::path m_path_;
	};

	inline void Mesh::Initialize()
	{
		Load();

		Graphic::D3Device::CreateBuffer<VertexElement>(D3D11_BIND_VERTEX_BUFFER, m_vertices_.size(), m_vertex_buffer_.ReleaseAndGetAddressOf(), m_vertices_.data());
		Graphic::D3Device::CreateBuffer<UINT>(D3D11_BIND_INDEX_BUFFER, m_indices_.size(), m_index_buffer_.ReleaseAndGetAddressOf(), m_indices_.data());
	}

	inline void Mesh::Update()
	{
	}

	inline void Mesh::Render()
	{
		Graphic::RenderPipeline::BindVertexBuffer(m_vertex_buffer_.Get());
		Graphic::RenderPipeline::BindIndexBuffer(m_index_buffer_.Get());
	}
}
