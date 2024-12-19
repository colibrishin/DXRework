#pragma once
#include "GraphicInterface.h"

#include "Source/Runtime/Core/Resource/Public/Resource.h"

#ifdef PHYSX_ENABLED
namespace physx
{
	class PxSDFDesc;
	class PxTriangleMeshGeometry;
	class PxTriangleMesh;
}
#endif

namespace Engine 
{
	struct PrimitiveMesh;

	struct MESH_API AccelStructBuffer
	{
		Unique<GraphicMemoryPool> instanceDescPool;
		Unique<GraphicMemoryPool> resultPool;
		Unique<GraphicMemoryPool> scratchPool;

		bool empty = true;
	};
}

namespace Engine::Resources
{
	class MESH_API Mesh : public Abstracts::Resource
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

		[[nodiscard]] BoundingOrientedBox     GetBoundingBox() const;
		[[nodiscard]] size_t                  GetIndexCount() const;
		[[nodiscard]] const VertexCollection& GetVertexCollection() const;
		[[nodiscard]] const IndexCollection&  GetIndexCollection() const;

		void                     OnDeserialized() override;
		void                     OnSerialized() override;

#if CFG_RAYTRACING
		const AccelStructBuffer&               GetBLAS() const;
#endif
		
		[[nodiscard]] const IStructuredBufferType<Graphics::VertexElement>& GetVertexStructuredBuffer() const;

		RESOURCE_SELF_INFER_GETTER_DECL(Mesh)

	protected:
		Mesh();
		friend class Components::Collider;
		friend struct PrimitiveMesh;
		
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

		VertexCollection m_vertices_;
		IndexCollection  m_indices_;
		BoundingOrientedBox m_bounding_box_;

		Unique<PrimitiveMesh> m_primitive_mesh_;
		Unique<IStructuredBufferType<Graphics::VertexElement>> m_vertex_buffer_structured_;

#if CFG_RAYTRACING
		AccelStructBuffer m_blas_;
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

namespace Engine
{
	struct MESH_API PrimitiveMesh
	{
		virtual      ~PrimitiveMesh() = default;
		virtual void Generate(const Resources::Mesh* mesh) = 0;

	protected:
		virtual void SetNativeVertexBuffer(void* buffer)
		{
			m_vertex_buffer_ = buffer;
		}

		virtual void SetNativeIndexBuffer(void* buffer)
		{
			m_index_buffer_ = buffer;
		}

		static IStructuredBufferType<Graphics::VertexElement>& GetVertexStructuredBuffer(const Resources::Mesh* mesh)
		{
			return *mesh->m_vertex_buffer_structured_;
		}

#if CFG_RAYTRACING
		static AccelStructBuffer& GetAccelStructBuffer(const Resources::Mesh* mesh)
		{
			return mesh->m_blas_;
		}
#endif
		
	private:
		void* m_vertex_buffer_ = nullptr;
		void* m_index_buffer_ = nullptr;
	};
}