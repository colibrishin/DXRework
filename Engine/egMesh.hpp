#pragma once

#define TINYOBJLOADER_IMPLEMENTATION

#include <d3d11.h>
#include <filesystem>
#include <vector>
#include <SimpleMath.h>
#include <wrl/client.h>

#include "egCommon.hpp"
#include "egDXCommon.h"
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
		void Render(const float dt) override;
		void ReadOBJFile();

		const std::vector<Shape>& GetShapes() { return m_vertices_; }
		const std::vector<const Vector3*>& GetVertices() { return m_flatten_vertices_; }

		UINT GetIndexCount() const { return static_cast<UINT>(m_indices_.size()); }

	protected:
		SERIALIZER_ACCESS

		friend class Component::Collider;

		Mesh(std::filesystem::path path) : Resource(std::move(path), RESOURCE_PRIORITY_MESH)
		{
		}

		void Load_INTERNAL() final;
		virtual void Load_CUSTOM() = 0;
		void Unload_INTERNAL() override;

		static void GenerateTangentBinormal(const Vector3& v0, const Vector3& v1, const Vector3& v2, const Vector2& uv0, const Vector2& uv1, const Vector2& uv2, Vector3& tangent, Vector3& binormal);
		void UpdateTangentBinormal();

		std::vector<Shape> m_vertices_;
		std::vector<const Vector3*> m_flatten_vertices_;
		std::vector<IndexCollection> m_indices_;

		VertexBufferCollection m_vertex_buffers_;
		IndexBufferCollection m_index_buffers_;

		D3D11_PRIMITIVE_TOPOLOGY m_topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	private:
		std::filesystem::path m_path_;
	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Mesh)