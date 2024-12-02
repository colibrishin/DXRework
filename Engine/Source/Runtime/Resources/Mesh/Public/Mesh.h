#pragma once
#include "Source/Runtime/Abstracts/CoreResource/Public/Resource.h"

#if defined(USE_DX12)
#include "Source/Runtime/StructuredBufferDX12/Public/StructuredBufferDX12.hpp"
#include "Source/Runtime/StructuredBufferDX12/Public/StructuredBufferMemoryPoolDX12.hpp"
#endif

#ifdef PHYSX_ENABLED
namespace physx
{
	class PxSDFDesc;
	class PxTriangleMeshGeometry;
	class PxTriangleMesh;
}
#endif

#if defined(USE_DX12)
namespace Engine 
{
	struct AccelStructBuffer
	{
		Graphics::GraphicMemoryPool<
			D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT,
			D3D12_HEAP_TYPE_UPLOAD,
			D3D12_RESOURCE_FLAG_NONE,
			D3D12_RESOURCE_STATE_GENERIC_READ> instanceDescPool;

		Graphics::GraphicMemoryPool<
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE> resultPool;

		Graphics::GraphicMemoryPool<
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS> scratchPool;

		bool empty = true;
	};
}
#endif

namespace Engine::Resources
{
	class Mesh : public Engine::Abstracts::Resource
	{
	public:
		RESOURCE_T(RES_T_MESH)

		Mesh(const VertexCollection& shape, const IndexCollection& indices);
		~Mesh() override;
		void Initialize() override;
		void PostUpdate(const float& dt) override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void FixedUpdate(const float& dt) override;

		BoundingOrientedBox GetBoundingBox() const;

		size_t                  GetIndexCount() const;
		const VertexCollection& GetVertexCollection() const;

		void                     OnDeserialized() override;
		void                     OnSerialized() override;

#if defined(USE_DX12)
		D3D12_VERTEX_BUFFER_VIEW GetVertexView() const;
		D3D12_INDEX_BUFFER_VIEW  GetIndexView() const;

#if CFG_RAYTRACING
		const AccelStructBuffer&               GetBLAS() const;
#endif
		const Graphics::StructuredBuffer<Graphics::VertexElement>& GetVertexStructuredBuffer() const;
		ID3D12Resource*                        GetIndexBuffer() const;
#endif

		RESOURCE_SELF_INFER_GETTER_DECL(Mesh)

	protected:
		SERIALIZE_DECL
		Mesh();

		friend class Components::Collider;

		void         Load_INTERNAL() final;
		virtual void Load_CUSTOM();
		void         Unload_INTERNAL() override;

		static void __vectorcall GenerateTangentBinormal(
			const Vector3& v0, const Vector3&  v1,
			const Vector3& v2, const Vector2&  uv0,
			const Vector2& uv1, const Vector2& uv2,
			Vector3&       tangent, Vector3&   binormal
		);
		void UpdateTangentBinormal();

	protected:
		VertexCollection m_vertices_;
		IndexCollection  m_indices_;
		BoundingOrientedBox m_bounding_box_;

#if defined(USE_DX12)
		ComPtr<ID3D12Resource> m_vertex_buffer_;
		ComPtr<ID3D12Resource> m_index_buffer_;
		ComPtr<ID3D12Resource> m_vertex_buffer_upload_;
		ComPtr<ID3D12Resource> m_index_buffer_upload_;

		Graphics::StructuredBuffer<Graphics::VertexElement> m_vertex_buffer_structured_;

		D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view_;
		D3D12_INDEX_BUFFER_VIEW  m_index_buffer_view_;

#if CFG_RAYTRACING
		AccelStructBuffer m_blas_;

		ComPtr<ID3D12Resource> m_raytracing_vertex_buffer_;
		ComPtr<ID3D12Resource> m_raytracing_index_buffer_;
		ComPtr<ID3D12Resource> m_raytracing_vertex_buffer_upload_;
		ComPtr<ID3D12Resource> m_raytracing_index_buffer_upload_;
#endif
#endif

#ifdef PHYSX_ENABLED
	public:
		physx::PxTriangleMeshGeometry* GetPhysXGeometry() const;
		physx::PxTriangleMesh* GetPhysXMesh() const;

	protected:
		physx::PxSDFDesc* m_px_sdf_;
		physx::PxTriangleMesh* m_px_mesh_;
#endif
	};
} // namespace Engine::Resources

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Mesh)
