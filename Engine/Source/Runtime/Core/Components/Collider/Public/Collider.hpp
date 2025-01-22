#pragma once

#define _USE_MATH_DEFINES
#include <math.h>
#include <set>

#include "Source/Runtime/Core/Component/Public/Component.h"
#include "Source/Runtime/Core/Delegation/Public/Delegation.hpp"
#include "Source/Runtime/Core/VertexElement/Public/VertexElement.hpp"
#include "Source/Runtime/Core/GenericBounding/Public/GenericBounding.hpp"

#ifdef PHYSX_ENABLED
namespace physx
{
	class PxRigidDynamic;
	class PxMaterial;
	class PxShape;
}
#endif

DEFINE_DELEGATE(OnCollisionEnter, Engine::Weak<Engine::Components::Collider>);
DEFINE_DELEGATE(OnCollisionEnd, Engine::Weak<Engine::Components::Collider>);

namespace Engine
{
	template <size_t Longitutde = 16, size_t Latitudue = 16>
	struct SphereGenerator
	{
	private:
		static constexpr IndexCollection GenerateIndices()
		{
			IndexCollection indices;
			
			/*
			*  Indices
			*  k1--k1+1
			*  |  / |
			*  | /  |
			*  k2--k2+1
			*/
			unsigned int k1, k2;
			for(int i = 0; i < Latitudue; ++i)
			{
				k1 = i * (Longitutde + 1);
				k2 = k1 + Longitutde + 1;
				// 2 Triangles per latitude block excluding the first and last longitudes blocks
				for (int j = 0; j < Longitutde; ++j, ++k1, ++k2)
				{
					if (i != 0)
					{
						indices.push_back(k1);
						indices.push_back(k2);
						indices.push_back(k1 + 1);
					}

					if (i != (Latitudue - 1))
					{
						indices.push_back(k1 + 1);
						indices.push_back(k2);
						indices.push_back(k2 + 1);
					}
				}
			}

			return indices;
		}
		
		static constexpr VertexCollection GenerateSphereSmooth()
		{
			float radius = 1;
			
			VertexCollection collection;
			float lengthInv = 1.0f / radius;    // normal

			// Generate vertices
			for (int lat = 0; lat <= Latitudue; ++lat)
			{
				float theta = lat * M_PI / Latitudue; // Polar angle
				float sinTheta = std::sin(theta);
				float cosTheta = std::cos(theta);

				for (int lon = 0; lon <= Longitutde; ++lon)
				{
					float phi = (float)lon * 2.0f * M_PI / Longitutde; // Azimuthal angle
					float sinPhi = std::sin(phi);
					float cosPhi = std::cos(phi);

					// Calculate vertex position
					float x = sinTheta * cosPhi;
					float y = cosTheta;
					float z = sinTheta * sinPhi;
					
					float u = (float)lon/Longitutde;
					float v = (float)lat/Latitudue;

					// normalized vertex normal
					float nx = x * lengthInv;
					float ny = y * lengthInv;
					float nz = z * lengthInv;

					// Normal is the same as position for unit sphere
					collection.push_back(Graphics::VertexElement(
						Vector3(x, y, z),
						Color(0.f, 0.f, 0.f, 1.f),
						Vector2(u, v),
						Vector3(nx, ny, nz),
						Vector3(0.f, 0.f, 0.f),
						Vector3(0.f, 0.f, 0.f),
						Graphics::VertexBoneElement()));
				}
			}

		    return collection;
		}
		
	public:
		static constexpr uint64_t s_vertices_count = (Latitudue + 1) * (Longitutde + 1);
		static constexpr uint64_t s_indices_count = 6 * (Latitudue * Longitutde);
		
		static constexpr IndexCollection GetSphereIndices()
		{
			return GenerateIndices();
		}

		static constexpr VertexCollection GetSphereVertices()
		{
			return GenerateSphereSmooth();
		}
	};

	struct CubeGenerator
	{
	private:
		static constexpr Vector3 vertices[8] =
		{
			{-1, -1, -1},
			{1, -1, -1},
			{1, 1, -1},
			{-1, 1, -1},
			{-1, -1, 1},
			{1, -1, 1},
			{1, 1, 1},
			{-1, 1, 1}
		};

		static constexpr Vector2 texCoords[4] =
		{
			{0, 0},
			{1, 0},
			{1, 1},
			{0, 1}
		};

		static constexpr Vector3 normals[6] =
		{
			{0, 0, 1},
			{1, 0, 0},
			{0, 0, -1},
			{-1, 0, 0},
			{0, 1, 0},
			{0, -1, 0}
		};

		static constexpr IndexCollection stock_indices =
		{
			0, 1, 3, 3, 1, 2,
			1, 5, 2, 2, 5, 6,
			5, 4, 6, 6, 4, 7,
			4, 0, 7, 7, 0, 3,
			3, 2, 7, 7, 2, 6,
			4, 5, 0, 0, 5, 1
		};

		static constexpr uint32_t texInds[6] = {0, 1, 3, 3, 1, 2};

		static constexpr VertexCollection GenerateCubeVertices()
		{
			VertexCollection collection(36);
			const IndexCollection cube_indices = GetCubeIndices();

			for (size_t i = 0; i < 36; ++i)
			{
				collection[i] = Graphics::VertexElement
				(
					vertices[cube_indices[i]],
					{0.f, 0.f, 0.f, 1.f},
					texCoords[texInds[i % 4]],
					normals[cube_indices[i / 6]],
					Vector3(0.f, 0.f, 0.f),
					Vector3(0.f, 0.f, 0.f),
					Graphics::VertexBoneElement()
				);
			}

			return collection;
		}

	public:
		static constexpr IndexCollection GetCubeIndices()
		{
			return stock_indices;
		}
	
		static constexpr VertexCollection GetCubeVertices()
		{
			return GenerateCubeVertices();
		}
	};
	
	constexpr const char* s_stock_shape_names[] = 
	{
		"Cube",
		"Sphere",
	};
}

namespace Engine::Components
{
	using namespace DirectX;

	class ENGINE_CORE_API Collider final : public Engine::Abstracts::Component
	{
	public:
		INLINE_COMPILE_TIME_TYPENAME(Collider)
		COMPONENT_T(COM_T_COLLIDER);

		DelegateOnCollisionEnter onCollisionEnter;
		DelegateOnCollisionEnd onCollisionEnd;

		Collider(const Weak<Engine::Abstracts::ObjectBase>& owner);
		~Collider() override;

		void FromMatrix(const Matrix& mat);

		void SetType(eBoundingType type);
		void SetMass(float mass);
		void SetBoundingBox(const BoundingOrientedBox& bounding);
		void SetVertices(const VertexCollection& vertex_collection);

		static bool Intersects(const Strong<Collider>& lhs, const Strong<Collider>& rhs, const Vector3& dir);
		static bool Intersects(const Strong<Collider>& lhs, const Strong<Collider>& rhs, float epsilon = 1e-03);
		static bool Intersects(const Strong<Collider>& lhs, const Strong<Collider>& rhs, float dist, const Vector3& dir);
		static bool ContainsBy(const Strong<Collider>& test, const Strong<Collider>& container);

		void AddCollidedObject(GlobalEntityID id);
		void RemoveCollidedObject(GlobalEntityID id);

		[[nodiscard]] bool                            IsCollidedObject(GlobalEntityID id) const;
		[[nodiscard]] const std::set<GlobalEntityID>& GetCollidedObjects() const;

		[[nodiscard]] float      GetMass() const;
		[[nodiscard]] float      GetInverseMass() const;
		[[nodiscard]] XMFLOAT3X3 GetInertiaTensor() const;

		[[nodiscard]] eBoundingType GetType() const;

		[[nodiscard]] const std::vector<Graphics::VertexElement>& GetVertices() const;
		[[nodiscard]] Matrix                                      GetWorldMatrix() const;
		[[nodiscard]] virtual Matrix                              GetLocalMatrix() const;

		void Initialize() override;
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void PostUpdate(const float dt) override;
		void FixedUpdate(const float dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		[[nodiscard]] GenericBounding<> GetBounding() const;

		template <typename T>
		[[nodiscard]] T GetBounding() const
		{
			if constexpr (std::is_same_v<T, BoundingOrientedBox>)
			{
				return m_boundings_.As<BoundingOrientedBox>(GetWorldMatrix());
			}
			else if constexpr (std::is_same_v<T, BoundingSphere>)
			{
				return m_boundings_.As<BoundingSphere>(GetWorldMatrix());
			}
			else
			{
				static_assert("Invalid type");
				throw std::exception("Invalid type");
			}
		}

		template <typename T>
		[[nodiscard]] T GetBoundingLocal() const
		{
			if constexpr (std::is_same_v<T, BoundingOrientedBox>)
			{
				return m_boundings_.As<BoundingOrientedBox>(GetLocalMatrix());
			}
			else if constexpr (std::is_same_v<T, BoundingSphere>)
			{
				return m_boundings_.As<BoundingSphere>(GetLocalMatrix());
			}
			else
			{
				static_assert("Invalid type");
				throw std::exception("Invalid type");
			}
		}

	protected:
		Collider();

	private:
		COMP_CLONE_DECL
		friend class Managers::LerpManager;

		static VertexCollection s_cube_vertices_;
		static IndexCollection s_cube_indices_;
		static VertexCollection s_sphere_vertices_;
		static IndexCollection s_sphere_indices_;

		static void InitializeStockVertices();

		void UpdateInertiaTensor();
		void GenerateInertiaCube();
		void GenerateInertiaSphere();

		eBoundingType m_type_;
		GenericBounding<> m_boundings_;

		float m_mass_;

		// Theoretically we could fallback the model by using the raw resource
		// path, however it stores the meta data for the consistency.
		std::set<GlobalEntityID> m_collided_objects_;

		Vector3          m_inverse_inertia_;
		XMFLOAT3X3       m_inertia_tensor_;
		Matrix           m_local_matrix_;
		VertexCollection m_vertices_;

#ifdef PHYSX_ENABLED
	private:
		friend class Rigidbody;

		void UpdatePhysXShape();
		void CleanupPhysX();
		void UpdateShapeFilter(eLayerType other) const;
		void UpdateShapeFilter(const eLayerType left, const eLayerType right) const;

		[[nodiscard]] physx::PxRigidDynamic* GetPhysXRigidbody() const;
		void ResetRigidbody(Weak<Component> component);

		Matrix m_previous_world_matrix_;
		Vector3 m_previous_scale_;

		inline static physx::PxTriangleMesh* s_px_cube_stock_ = nullptr;
		inline static physx::PxTriangleMesh* s_px_sphere_stock_ = nullptr;

		inline static physx::PxSDFDesc* s_px_cube_sdf_ = nullptr;
		inline static physx::PxSDFDesc* s_px_sphere_sdf = nullptr;

		physx::PxMaterial* m_px_material_;
		physx::PxRigidDynamic* m_px_rb_static_;
		std::vector<physx::PxShape*> m_px_meshes_;
#endif
	};
} // namespace Engine::Components
