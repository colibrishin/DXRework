#pragma once

#define TINYOBJLOADER_IMPLEMENTATION

#include <d3d11.h>
#include <filesystem>
#include <vector>
#include <list>
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

	using Shape = std::vector<VertexElement>;
	using IndexCollection = std::vector<UINT>;
	using VertexBufferCollection = std::vector<ComPtr<ID3D11Buffer>>;
	using IndexBufferCollection = std::vector<ComPtr<ID3D11Buffer>>;

	class Mesh : public Abstract::Resource
	{
	public:
		~Mesh() override = default;
		void Initialize() override;
		void Render() override;
		void ReadOBJFile();

		UINT GetIndexCount() const { return m_indices_.size(); }

	protected:
		friend class Component::Collider;

		Mesh(std::filesystem::path path) : Resource(std::move(path), RESOURCE_PRIORITY_MESH)
		{
		}

		void Load() override;
		void Unload_INTERNAL() override;

		static void GenerateTangentBinormal(const Vector3& v0, const Vector3& v1, const Vector3& v2, const Vector2& uv0, const Vector2& uv1, const Vector2& uv2, Vector3& tangent, Vector3& binormal);
		void UpdateTangentBinormal();

		std::vector<Shape> m_vertices_;
		std::vector<IndexCollection> m_indices_;

		VertexBufferCollection m_vertex_buffers_;
		IndexBufferCollection m_index_buffers_;

		D3D11_PRIMITIVE_TOPOLOGY m_topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	private:
		std::filesystem::path m_path_;
	};

	inline void Mesh::Initialize()
	{
	}

	inline void Mesh::Render()
	{
		for (int i = 0; i < m_vertex_buffers_.size(); ++i)
		{
			Graphic::RenderPipeline::BindVertexBuffer(m_vertex_buffers_[i].Get());
			Graphic::RenderPipeline::BindIndexBuffer(m_index_buffers_[i].Get());
			Graphic::RenderPipeline::SetTopology(m_topology);
			Graphic::RenderPipeline::DrawIndexed(m_indices_[i].size());
		}

		Graphic::RenderPipeline::BindResource(SR_TEXTURE, nullptr);
	}

	inline void Mesh::Load()
	{
		Resource::Load();

		if (!GetPath().empty())
		{
			if (GetPath().extension() == ".obj")
			{
				ReadOBJFile();
			}
		}

		UpdateTangentBinormal();

		m_vertex_buffers_.resize(m_vertices_.size());
		m_index_buffers_.resize(m_indices_.size());

		for (int i = 0; i < m_vertex_buffers_.size(); ++i)
		{
			Graphic::D3Device::CreateBuffer<VertexElement>(D3D11_BIND_VERTEX_BUFFER, m_vertices_[i].size(),
		                                               m_vertex_buffers_[i].ReleaseAndGetAddressOf(), m_vertices_[i].data());
		}

		for(int i = 0; i < m_index_buffers_.size(); ++i)
		{
			Graphic::D3Device::CreateBuffer<UINT>(D3D11_BIND_INDEX_BUFFER, m_indices_[i].size(),
			                                      m_index_buffers_[i].ReleaseAndGetAddressOf(), m_indices_[i].data());
		}
	}

	inline void Mesh::Unload_INTERNAL()
	{
		for (const auto& buffer : m_vertex_buffers_)
		{
			buffer->Release();
		}

		for (const auto& buffer : m_index_buffers_)
		{
			buffer->Release();
		}
	}
}
