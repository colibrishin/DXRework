#pragma once

#include <set>

#include "Source/Runtime/Core/Component/Public/Component.h"
#include "Source/Runtime/Core/Delegation/Public/Delegation.hpp"
#include "Source/Runtime/Core/VertexElement/Public/VertexElement.hpp"
#include "Source/Runtime/Core/GenericBounding/Public/GenericBounding.hpp"

#include "Collider.generated.h"

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
	constexpr const char* s_stock_shape_names[] = 
	{
		"Cube",
		"Sphere",
	};
}

namespace Engine::Components
{
	ECLASS()
	class ENGINE_CORE_API Collider final : public Engine::Abstracts::Component
	{
		GENERATE_BODY
	public:
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
		void OnUIUpdate(UIContext* const context, const float dt) override;
		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void PostUpdate(const float dt) override;
		void FixedUpdate(const float dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;
		eComponentUpdatePriority GetUpdatePriority() const override;

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

		EPROPERTY()
		eBoundingType m_type_;
		
		EPROPERTY()
		GenericBounding<> m_boundings_;

		EPROPERTY()
		float m_mass_;

		// Theoretically we could fallback the model by using the raw resource
		// path, however it stores the meta data for the consistency.
		std::set<GlobalEntityID> m_collided_objects_;

		EPROPERTY()
		Vector3          m_inverse_inertia_;
		
		EPROPERTY()
		XMFLOAT3X3       m_inertia_tensor_;
		
		EPROPERTY()
		Matrix           m_local_matrix_;
		
		EPROPERTY()
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
