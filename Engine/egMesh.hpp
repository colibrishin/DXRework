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
#include "egResource.hpp"


namespace Engine::Component
{
	class Collider;
}

namespace Engine::Resources
{
	using namespace DirectX::SimpleMath;
	using Microsoft::WRL::ComPtr;

	class Mesh : public Abstract::Resource
	{
	public:
		~Mesh() override = default;
		void Initialize() override;
		void Render() override;

		UINT GetIndexCount() const { return m_indices_.size(); }

	protected:
		friend class Component::Collider;

		Mesh(std::filesystem::path path) : Resource(std::move(path), RESOURCE_PRIORITY_MESH)
		{
		}

		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

		std::vector<VertexElement> m_vertices_;
		std::vector<UINT> m_indices_;

		ComPtr<ID3D11Buffer> m_vertex_buffer_;
		ComPtr<ID3D11Buffer> m_index_buffer_;

		D3D11_PRIMITIVE_TOPOLOGY m_topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	private:
		std::filesystem::path m_path_;
	};

	inline void Mesh::Initialize()
	{
	}

	inline void Mesh::Render()
	{
		Graphic::RenderPipeline::BindVertexBuffer(m_vertex_buffer_.Get());
		Graphic::RenderPipeline::BindIndexBuffer(m_index_buffer_.Get());
		Graphic::RenderPipeline::SetTopology(m_topology);
		Graphic::RenderPipeline::DrawIndexed(m_indices_.size());
	}

	inline void Mesh::Load_INTERNAL()
	{
		Graphic::D3Device::CreateBuffer<VertexElement>(D3D11_BIND_VERTEX_BUFFER, m_vertices_.size(),
		                                               m_vertex_buffer_.ReleaseAndGetAddressOf(), m_vertices_.data());
		Graphic::D3Device::CreateBuffer<UINT>(D3D11_BIND_INDEX_BUFFER, m_indices_.size(),
		                                      m_index_buffer_.ReleaseAndGetAddressOf(), m_indices_.data());
	}

	inline void Mesh::Unload_INTERNAL()
	{
		m_vertex_buffer_->Release();
		m_index_buffer_->Release();
	}
}
