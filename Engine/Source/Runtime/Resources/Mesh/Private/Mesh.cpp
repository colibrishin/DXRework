#ifdef PHYSX_ENABLED
#include <PxPhysics.h>
#include <cooking/PxCooking.h>
#include <extensions/PxTriangleMeshExt.h>
#include <geometry/PxTriangleMeshGeometry.h>
#endif

#include "../Public/Mesh.h"

#include <algorithm>
#include <execution>
#include <directxtk12/BufferHelpers.h>

#include "Components/Collider/Public/Collider.h"

#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.h"
#include "Source/Runtime/Core/SIMDExtension/Public/SIMDExtension.hpp"
#include "Source/Runtime/Core/VertexElement/Public/VertexElement.hpp"

namespace Engine::Resources
{
	size_t Mesh::GetIndexCount() const
	{
		return m_indices_.size();
	}

	const VertexCollection& Mesh::GetVertexCollection() const
	{
		return m_vertices_;
	}

	const IndexCollection& Mesh::GetIndexCollection() const
	{
		return m_indices_;
	}

	void Mesh::UpdateCollider(const Weak<Components::Collider>& w_collider) const
	{
		if (const Strong<Components::Collider>& collider = w_collider.lock())
		{
			collider->SetBoundingBox(GetBoundingBox());
			collider->SetVertices(m_vertices_);
		}
	}

	Mesh::Mesh(const VertexCollection& shape, const IndexCollection& indices)
		: Resource(""),
		  m_vertices_(shape),
		  m_indices_(indices) {}

	Mesh::~Mesh()
	{
#ifdef PHYSX_ENABLED
		if (m_px_mesh_)
		{
			m_px_mesh_->release();
			m_px_mesh_ = nullptr;
		}

		if (m_px_sdf_)
		{
			delete m_px_sdf_;
			m_px_sdf_ = nullptr;
		}
#endif
	}

	void __vectorcall Mesh::GenerateTangentBinormal(
		const Vector3& v0, const Vector3&  v1,
		const Vector3& v2, const Vector2&  uv0,
		const Vector2& uv1, const Vector2& uv2,
		Vector3&       tangent, Vector3&   binormal
	)
	{
		const Vector3 edge1 = v1 - v0;
		const Vector3 edge2 = v2 - v0;

		const Vector2 deltaUV1 = {uv1.x - uv0.x, uv2.x - uv0.x};
		const Vector2 deltaUV2 = {uv1.y - uv0.y, uv2.y - uv0.y};

		const float delta       = (deltaUV1.x * deltaUV2.y) - (deltaUV1.y * deltaUV2.x);
		const float denominator = 1.0f / delta;

		tangent.x = denominator * (deltaUV1.y * edge1.x - deltaUV1.x * edge2.x);
		tangent.y = denominator * (deltaUV1.y * edge1.y - deltaUV1.x * edge2.y);
		tangent.z = denominator * (deltaUV1.y * edge1.z - deltaUV1.x * edge2.z);

		binormal.x = denominator * (deltaUV2.x * edge2.x - deltaUV2.y * edge1.x);
		binormal.y = denominator * (deltaUV2.x * edge2.y - deltaUV2.y * edge1.y);
		binormal.z = denominator * (deltaUV2.x * edge2.z - deltaUV2.y * edge1.z);

		tangent.Normalize();
		binormal.Normalize();
	}

	void Mesh::UpdateTangentBinormal()
	{
		struct FacePair
		{
			Graphics::VertexElement* o[3];
		};

		const auto& indices = m_indices_;
		if (indices.size() % 3 != 0)
		{
			return;
		}

		std::vector<FacePair> faces;

		for (size_t i = 0; i < indices.size(); i += 3)
		{
			const auto& i0 = indices[i];
			const auto& i1 = indices[i + 1];
			const auto& i2 = indices[i + 2];

			FacePair face = {{&m_vertices_[i0], &m_vertices_[i1], &m_vertices_[i2]}};

			faces.push_back(face);
		}

		std::mutex commit_lock;

		std::for_each
				(
				 std::execution::par, faces.begin(), faces.end(),
				 [&](const FacePair& face)
				 {
					 Vector3 tangent;
					 Vector3 binormal;

					 GenerateTangentBinormal
							 (
							  face.o[0]->position, face.o[1]->position,
							  face.o[2]->position, face.o[0]->texCoord,
							  face.o[1]->texCoord, face.o[2]->texCoord, tangent,
							  binormal
							 );

					 {
						 std::lock_guard cl(commit_lock);
						 face.o[0]->tangent = tangent;
						 face.o[1]->tangent = tangent;
						 face.o[2]->tangent = tangent;

						 face.o[0]->binormal = binormal;
						 face.o[1]->binormal = binormal;
						 face.o[2]->binormal = binormal;
					 }
				 }
				);
	}

#ifdef PHYSX_ENABLED
	physx::PxTriangleMesh* Mesh::GetPhysXMesh() const
	{
		return m_px_mesh_;
	}
#endif

	void Mesh::PreUpdate(const float dt) {}

	void Mesh::Update(const float dt) {}

	void Mesh::FixedUpdate(const float dt) {}

	void Mesh::OnDeserialized()
	{
		Resource::OnDeserialized();
	}

	void Mesh::OnSerialized()
	{
		Resource::OnSerialized();
	}

#if CFG_RAYTRACING
	const AccelStructBuffer& Mesh::GetBLAS() const
	{
		return m_blas_;
	}
#endif

	StructuredBufferTypeProxy<Graphics::VertexElement>& Mesh::GetVertexStructuredBuffer() const
	{
		return *m_vertex_buffer_structured_;
	}

	void Mesh::Initialize() {}

	void Mesh::PostUpdate(const float dt) {}

	BoundingOrientedBox Mesh::GetBoundingBox() const
	{
		return m_bounding_box_;
	}

	Mesh::Mesh()
		: Resource("") {}

	void Mesh::Load_INTERNAL()
	{
		Load_CUSTOM();

		UpdateTangentBinormal();

		std::vector<Vector3> pure_vertices;
		for (const auto& vertex : m_vertices_)
		{
			pure_vertices.push_back(vertex.position);
		}

		BoundingOrientedBox::CreateFromPoints
				(
				 m_bounding_box_, m_vertices_.size(), pure_vertices.data(),
				 sizeof(Vector3)
				);

		m_primitive_mesh_ = Unique<PrimitiveMesh>(GraphicInterfaceAccessor::GetInterface().GetNewPrimitiveMesh());
		m_primitive_mesh_->Generate(this);
	}

	void Mesh::Load_CUSTOM() {}

	void Mesh::Unload_INTERNAL()
	{
		m_primitive_mesh_.reset();
		m_vertex_buffer_structured_.reset();

#if CFG_RAYTRACING
		if (m_blas_.resultPool.GetResource())
		{
			m_blas_.resultPool.Release();
		}
		if (m_blas_.scratchPool.GetResource())
		{
			m_blas_.scratchPool.Release();
		}
		if (m_raytracing_vertex_buffer_)
		{
			m_raytracing_vertex_buffer_->Release();
		}
		if (m_raytracing_index_buffer_)
		{
			m_raytracing_index_buffer_->Release();
		}
		if (m_raytracing_vertex_buffer_upload_)
		{
			m_raytracing_vertex_buffer_upload_->Release();
		}
		if (m_raytracing_index_buffer_upload_)
		{
			m_raytracing_index_buffer_upload_->Release();
		}
		if (m_blas_.instanceDescPool.GetResource())
		{
			m_blas_.instanceDescPool.Release();
		}
#endif

#ifdef PHYSX_ENABLED
		if (m_px_mesh_)
		{
			m_px_mesh_->release();
			m_px_mesh_ = nullptr;
		}

		if (m_px_sdf_)
		{
			delete m_px_sdf_;
			m_px_sdf_ = nullptr;
		}
#endif
	}
} // namespace Engine::Resources
