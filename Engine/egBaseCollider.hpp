#pragma once
#include "egCommon.hpp"
#include "egComponent.h"
#include "egGenericBounding.hpp"
#include "egHelper.hpp"
#include "egDelegate.hpp"
#include "egTransform.h"

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

namespace Engine::Components
{
	using namespace DirectX;

	class Collider final : public Abstract::Component
	{
	public:
		COMPONENT_T(COM_T_COLLIDER);

		DelegateOnCollisionEnter onCollisionEnter;
		DelegateOnCollisionEnd onCollisionEnd;

		Collider(const WeakObjectBase& owner);
		~Collider() override;

		void FromMatrix(const Matrix& mat);

		void SetType(eBoundingType type);
		void SetMass(float mass);

		void SetBoundingBox(const BoundingOrientedBox& bounding);
		void SetShape(const WeakModel& model);

		static bool Intersects(const StrongCollider& lhs, const StrongCollider& rhs, const Vector3& dir);
		static bool Intersects(const StrongCollider& lhs, const StrongCollider& rhs, float epsilon = g_epsilon);
		static bool Intersects(const StrongCollider& lhs, const StrongCollider& rhs, float dist, const Vector3& dir);

		static bool ContainsBy(const StrongCollider& test, const StrongCollider& container);

		bool Intersects(const Vector3& start, const Vector3& dir, float distance, float& intersection) const;

		void AddCollidedObject(GlobalEntityID id);
		void RemoveCollidedObject(GlobalEntityID id);

		bool                            IsCollidedObject(GlobalEntityID id) const;
		const std::set<GlobalEntityID>& GetCollidedObjects() const;

		bool GetPenetration(
			const Collider& other, Vector3& normal,
			float&          depth
		) const;

		float      GetMass() const;
		float      GetInverseMass() const;
		XMFLOAT3X3 GetInertiaTensor() const;

		eBoundingType GetType() const;

		const std::vector<Graphics::VertexElement>& GetVertices() const;
		Matrix                                      GetWorldMatrix() const;
		virtual Matrix                              GetLocalMatrix() const;

		void Initialize() override;
		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PostUpdate(const float& dt) override;
		void FixedUpdate(const float& dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;
		void OnImGui() override;

		Physics::GenericBounding GetBounding() const;

		template <typename T>
		T GetBounding() const
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
		T GetBoundingLocal() const
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
		SERIALIZE_DECL
		COMP_CLONE_DECL

		friend class Manager::Physics::LerpManager;

		static void InitializeStockVertices();

		void UpdateInertiaTensor();
		void GenerateInertiaCube();
		void GenerateInertiaSphere();
		void VerifyMaterial(boost::weak_ptr<Resources::Material> weak);
		void UpdateByOwner(Weak<Component> component);
		void ResetToStockObject(Weak<Component> component);

		eBoundingType m_type_;
		std::string   m_shape_meta_path_str_;

		Physics::GenericBounding m_boundings_;

		float m_mass_;

		// Non-serialized
		inline static std::vector<Graphics::VertexElement> s_cube_stock_   = {};
		inline static std::vector<Graphics::VertexElement> s_sphere_stock_ = {};
		inline static std::vector<UINT> s_cube_stock_indices_ = {};
		inline static std::vector<UINT> s_sphere_stock_indices_ = {};
		static constexpr const char* s_stock_shape_names[] = 
		{
			"Cube",
			"Sphere",
		};

		// Theoretically we could fallback the model by using the raw resource
		// path, however it stores the meta data for the consistency.
		std::filesystem::path    m_shape_meta_path_;
		std::set<GlobalEntityID> m_collided_objects_;

		Vector3    m_inverse_inertia_;
		XMFLOAT3X3 m_inertia_tensor_;
		Matrix     m_local_matrix_;

		WeakModel m_shape_;

#ifdef PHYSX_ENABLED
	private:
		friend class Rigidbody;

		void UpdatePhysXShape();
		void CleanupPhysX();

		physx::PxRigidDynamic* GetPhysXRigidbody() const;
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
} // namespace Engine::Component

BOOST_CLASS_EXPORT_KEY(Engine::Components::Collider)
