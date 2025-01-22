#pragma once
#include "GraphicInterface.h"
#include "Source/Runtime/Core/Resource/Public/Resource.h"
#include "VertexElement/Public/VertexElement.h"
#include "ResourceManager/Public/ResourceManager.h"

#include "Mesh.generated.h"

#ifdef PHYSX_ENABLED
namespace physx
{
	class PxSDFDesc;
	class PxTriangleMeshGeometry;
	class PxTriangleMesh;
}
#endif

namespace Engine::Resources
{
	ECLASS(resource, serialize)
	class ENGINE_MESH_API Mesh : public Abstracts::Resource
	{
		GENERATE_BODY
	public:
		Mesh(const VertexCollection& shape, const IndexCollection& indices);
		~Mesh() override;
		void Initialize() override;
		void PostUpdate(const float dt) override;
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void FixedUpdate(const float dt) override;
		
		[[nodiscard]] BoundingOrientedBox     GetBoundingBox() const;
		[[nodiscard]] size_t                  GetIndexCount() const;
		[[nodiscard]] const VertexCollection& GetVertexCollection() const;
		[[nodiscard]] const IndexCollection&  GetIndexCollection() const;

		void UpdateCollider(const Weak<Components::Collider>& w_collider) const;

		void                     OnDeserialized() override;
		void                     OnSerialized() override;

#if CFG_RAYTRACING
		const AccelStructBuffer&               GetBLAS() const;
#endif
		
		[[nodiscard]] StructuredBufferTypeProxy<Graphics::VertexElement>& GetVertexStructuredBuffer() const;

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

		EPROPERTY()
		VertexCollection m_vertices_;
		
		EPROPERTY()
		IndexCollection  m_indices_;
		
		EPROPERTY()
		BoundingOrientedBox m_bounding_box_;

		Unique<PrimitiveMesh> m_primitive_mesh_;
		Unique<StructuredBufferTypeProxy<Graphics::VertexElement>> m_vertex_buffer_structured_;

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
