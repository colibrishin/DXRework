#include "pch.h"
#include "egBaseCollider.hpp"

#ifdef PHYSX_ENABLED
#include <PxMaterial.h>
#include <PxPhysics.h>
#include <PxRigidDynamic.h>
#include <PxRigidStatic.h>
#include <PxScene.h>
#include <extensions/PxRigidActorExt.h>
#include <extensions/PxDefaultSimulationFilterShader.h>
#endif

#include <imgui_stdlib.h>

#include <cooking/PxCooking.h>
#include <cooking/PxTriangleMeshDesc.h>

#include <extensions/PxDefaultStreams.h>

#include "egCollision.h"
#include "egCubeMesh.h"
#include "egD3Device.hpp"
#include "egGlobal.h"
#include "egResourceManager.hpp"
#include "egSceneManager.hpp"
#include "egShape.h"
#include "egSphereMesh.h"
#include "egTransform.h"

#include "egManagerHelper.hpp"

SERIALIZE_IMPL
(
 Engine::Components::Collider,
 _ARTAG(_BSTSUPER(Engine::Abstract::Component))
 _ARTAG(m_type_) _ARTAG(m_shape_meta_path_str_) _ARTAG(m_mass_)
 _ARTAG(m_boundings_)
)

namespace Engine::Components
{
	COMP_CLONE_IMPL(Collider)

	const std::vector<Graphics::VertexElement>& Collider::GetVertices() const
	{
		if (const auto model = m_shape_.lock())
		{
			return model->GetVertices();
		}

		if (m_type_ == BOUNDING_TYPE_BOX)
		{
			return s_cube_stock_;
		}
		if (m_type_ == BOUNDING_TYPE_SPHERE)
		{
			return s_sphere_stock_;
		}

		return {};
	}

	Matrix Collider::GetWorldMatrix() const
	{
		return GetLocalMatrix() * GetOwner().lock()->GetComponent<Transform>().lock()->GetWorldMatrix();
	}

	Matrix Collider::GetLocalMatrix() const
	{
		return m_local_matrix_;
	}

	void Collider::Initialize()
	{
		Component::Initialize();
		const auto& owner = GetOwner().lock();

		if (owner && !owner->GetComponent<Transform>().lock())
		{
			const auto _ = owner->AddComponent<Transform>().lock();
		}

		InitializeStockVertices();

#ifdef PHYSX_ENABLED
		// todo: move friction value from rb to collider
		m_px_material_ = GetPhysicsManager().GetPhysX()->createMaterial(0.1, 0.1, g_restitution_coefficient);
#endif
		
		owner->onComponentAdded.Listen(this, &Collider::UpdateByOwner);

		if (owner)
		{
			const auto& useStock = [this]()
				{
					const auto vtx_ptr = reinterpret_cast<Vector3*>(s_cube_stock_.data());
					m_boundings_.CreateFromPoints<BoundingBox>(s_cube_stock_.size(), vtx_ptr, sizeof(Graphics::VertexElement));

					if (m_type_ == BOUNDING_TYPE_BOX)
					{
						GenerateInertiaCube();
					}
					else if (m_type_ == BOUNDING_TYPE_SPHERE)
					{
						GenerateInertiaSphere();
					}

#ifdef PHYSX_ENABLED
					UpdatePhysXShape();
#endif
				};

			if (const auto& rc = owner->GetComponent<Base::RenderComponent>().lock())
			{
				rc->onMaterialChange.Listen(this, &Collider::VerifyMaterial);

				if (const auto& material = rc->GetMaterial().lock())
				{
					if (const auto& shape = material->GetResource<Resources::Shape>(0).lock())
					{
						SetShape(shape);
					}
				}
				else
				{
					useStock();
				}
			}
			else
			{
				useStock();
			}
		}

		UpdateInertiaTensor();
	}

	void Collider::InitializeStockVertices()
	{
		GeometricPrimitive::IndexCollection  index;
		GeometricPrimitive::VertexCollection vertex;

		if (s_cube_stock_.empty())
		{
			GeometricPrimitive::CreateCube(vertex, index, 1.f, false);

			for (const auto& v : vertex)
			{
				s_cube_stock_.push_back({v.position});
			}

			for (const auto& i : index)
			{
				s_cube_stock_indices_.push_back(i);
			}
		}
		if (s_sphere_stock_.empty())
		{
			GeometricPrimitive::CreateSphere(vertex, index, 1.f, 16, false);

			for (const auto& v : vertex)
			{
				s_sphere_stock_.push_back({v.position});
			}

			for (const auto& i : index)
			{
				s_sphere_stock_indices_.push_back(i);
			}
		}

#ifdef PHYSX_ENABLED
		const auto& cookMesh = [](const std::vector<Graphics::VertexElement>& vertices, const std::vector<UINT>& indices, physx::PxSDFDesc** built_sdf, physx::PxTriangleMesh** built_shape)
		{
			physx::PxTriangleMeshDesc mesh_desc;
			mesh_desc.points.count = vertices.size();
			mesh_desc.points.data = vertices.data();
			mesh_desc.points.stride = sizeof(Graphics::VertexElement);

			mesh_desc.triangles.count = indices.size() / 3;
			mesh_desc.triangles.stride = 3 * sizeof(UINT);
			mesh_desc.triangles.data = indices.data();

			// todo: prebuild
			*built_sdf = new physx::PxSDFDesc;
			(*built_sdf)->spacing = 0.125f;

			mesh_desc.sdfDesc = *built_sdf;

			physx::PxCookingParams cooking_params(GetPhysicsManager().GetPhysX()->getTolerancesScale());

			physx::PxTriangleMeshCookingResult::Enum result;
			physx::PxDefaultMemoryOutputStream out_stream;

			if (PxCookTriangleMesh(cooking_params, mesh_desc, out_stream, &result))
			{
				physx::PxDefaultMemoryInputData input_steam(
					out_stream.getData(), 
					out_stream.getSize());

				(*built_shape) = GetPhysicsManager().GetPhysX()->createTriangleMesh(input_steam);
				
				// todo: serialize sdf information after cooking
			}
			else
			{
				OutputDebugStringW(L"Failed to cook as stock collider triangle mesh by physx!");
			}
		};

		if (!s_px_cube_stock_)
		{
			cookMesh(s_cube_stock_, s_cube_stock_indices_, &s_px_cube_sdf_, &s_px_cube_stock_);
		}
		if (!s_px_sphere_stock_)
		{
			cookMesh(s_sphere_stock_, s_sphere_stock_indices_, &s_px_sphere_sdf, &s_px_sphere_stock_);
		}
#endif
	}

	void Collider::FromMatrix(const Matrix& mat)
	{
		m_local_matrix_ = mat;
	}

	void Collider::SetType(const eBoundingType type)
	{
		m_type_ = type;
		m_boundings_.SetType(type);

		if (m_type_ == BOUNDING_TYPE_BOX)
		{
			GenerateInertiaCube();
		}
		else if (m_type_ == BOUNDING_TYPE_SPHERE)
		{
			GenerateInertiaSphere();
		}

		UpdateInertiaTensor();
	}

	void Collider::SetMass(const float mass)
	{
		m_mass_ = mass;
	}

	void Collider::SetBoundingBox(const BoundingOrientedBox& bounding)
	{
		m_boundings_.UpdateFromBoundingBox(bounding);

		if (m_type_ == BOUNDING_TYPE_BOX)
		{
			GenerateInertiaCube();
		}
		else if (m_type_ == BOUNDING_TYPE_SPHERE)
		{
			GenerateInertiaSphere();
		}

		UpdateInertiaTensor();
	}

	void Collider::SetShape(const WeakModel& model)
	{
		if (const auto locked = model.lock())
		{
			m_shape_meta_path_ = locked->GetMetadataPath();
			m_shape_           = locked;

			BoundingOrientedBox obb;
			BoundingOrientedBox::CreateFromBoundingBox(obb, locked->GetBoundingBox());
			SetBoundingBox(obb);
#ifdef PHYSX_ENABLED
			UpdatePhysXShape();
#endif
		}
	}

	bool Collider::Intersects(const StrongCollider& lhs, const StrongCollider& rhs, const Vector3& dir)
	{
		return lhs->m_boundings_.Intersects(rhs->m_boundings_, lhs->GetWorldMatrix(), rhs->GetWorldMatrix(), dir);
	}

	bool Collider::Intersects(const StrongCollider& lhs, const StrongCollider& rhs, const float epsilon)
	{
		return lhs->m_boundings_.Intersects(rhs->m_boundings_, lhs->GetWorldMatrix(), rhs->GetWorldMatrix(), epsilon);
	}

	bool Collider::Intersects(
		const StrongCollider& lhs, const StrongCollider& rhs, const float dist, const Vector3& dir
	)
	{
		float      distance = 0.f;
		const auto test     = lhs->GetBounding().TestRay(rhs->GetBounding(), dir, distance);
		return test && distance <= dist;
	}

	bool Collider::ContainsBy(const StrongCollider& test, const StrongCollider& container)
	{
		return test->m_boundings_.ContainsBy
				(container->m_boundings_, test->GetWorldMatrix(), container->GetWorldMatrix());
	}

	bool Collider::Intersects(
		const Vector3& start, const Vector3& dir, float distance,
		float&         intersection
	) const
	{
		if (m_type_ == BOUNDING_TYPE_BOX)
		{
			const auto    box     = GetBounding<BoundingOrientedBox>();
			const Vector3 Extents = box.Extents;
			const auto    test    = Physics::Raycast::TestRayOBBIntersection
					(
					 start, dir, -Extents,
					 Extents,
					 GetWorldMatrix(),
					 intersection
					);

			return test && intersection <= distance;
		}
		if (m_type_ == BOUNDING_TYPE_SPHERE)
		{
			const auto sphere = GetBounding<BoundingSphere>();
			const auto test   = Physics::Raycast::TestRaySphereIntersection
					(
					 start, dir, sphere.Center,
					 sphere.Radius,
					 intersection
					);

			return test && intersection <= distance;
		}

		return false;
	}

	void Collider::AddCollidedObject(const GlobalEntityID id)
	{
		m_collided_objects_.insert(id);
	}

	void Collider::RemoveCollidedObject(const GlobalEntityID id)
	{
		m_collided_objects_.erase(id);
	}

	bool Collider::IsCollidedObject(const GlobalEntityID id) const
	{
		return m_collided_objects_.contains(id);
	}

	const std::set<GlobalEntityID>& Collider::GetCollidedObjects() const
	{
		return m_collided_objects_;
	}

	bool Collider::GetPenetration(
		const Collider& other, Vector3& normal,
		float&          depth
	) const
	{
		auto dir = other.GetWorldMatrix().Translation() - GetWorldMatrix().Translation();
		dir.Normalize();

		return Physics::GJK::GJKAlgorithm
				(
				 GetWorldMatrix(), other.GetWorldMatrix(), GetVertices(), other.GetVertices(), dir,
				 normal, depth
				);
	}

	float Collider::GetMass() const
	{
		return m_mass_;
	}

	float Collider::GetInverseMass() const
	{
		return 1.0f / m_mass_;
	}

	XMFLOAT3X3 Collider::GetInertiaTensor() const
	{
		return m_inertia_tensor_;
	}

	eBoundingType Collider::GetType() const
	{
		return m_type_;
	}

	Collider::Collider()
		: Component(COM_T_COLLIDER, {}),
		  m_type_(BOUNDING_TYPE_BOX),
		  m_boundings_(),
		  m_mass_(1.f),
		  m_shape_meta_path_(),
		  m_inertia_tensor_(),
		  m_local_matrix_(Matrix::Identity) {}

	void Collider::FixedUpdate(const float& dt)
	{
#ifdef PHYSX_ENABLED
		if (const auto& owner = GetOwner().lock())
		{
			if (const auto& tr = owner->GetComponent<Transform>().lock())
			{
				Matrix new_world = GetWorldMatrix();
				Vector3 position;
				Quaternion quaternion;
				Vector3 scale;

				new_world.Decompose(scale, quaternion, position);

				if (m_previous_scale_ != scale)
				{
					UpdatePhysXShape();
					return;
				}

				const physx::PxTransform px_transform
				{
					reinterpret_cast<const physx::PxVec3&>(position),
					reinterpret_cast<const physx::PxQuat&>(quaternion)
				};

				// casting sanity check
				if constexpr (g_debug)
				{
					const physx::PxVec3 px_pos = { position.x, position.y, position.z };
					const physx::PxQuat px_rot = { quaternion.x, quaternion.y, quaternion.z, quaternion.w };

					_ASSERT(px_transform.p == px_pos);
					_ASSERT(px_transform.q == px_rot);
				}

				m_px_rb_static_->setGlobalPose(px_transform);
				m_previous_world_matrix_ = new_world;
			}
		}
#endif
	}

	void Collider::OnSerialized()
	{
		if (const auto shape = m_shape_.lock())
		{
			Serializer::Serialize(shape->GetName(), shape);
			m_shape_meta_path_str_ = shape->GetMetadataPath().string();
			m_shape_meta_path_     = shape->GetMetadataPath();
		}
	}

	void Collider::OnDeserialized()
	{
		Component::OnDeserialized();

		InitializeStockVertices();
		m_shape_meta_path_ = m_shape_meta_path_str_;

		if (!m_shape_meta_path_.empty())
		{
			SetShape(Resources::Shape::GetByMetadataPath(m_shape_meta_path_));
		}

		if (m_type_ == BOUNDING_TYPE_BOX)
		{
			GenerateInertiaCube();
		}
		else if (m_type_ == BOUNDING_TYPE_SPHERE)
		{
			GenerateInertiaSphere();
		}

		UpdateInertiaTensor();
	}

	void Collider::OnImGui()
	{
		Component::OnImGui();
		ImGui::Indent(2);

		ImGui::InputInt("Collider Type", reinterpret_cast<int*>(&m_type_));

		ImGui::InputFloat("Collider Mass", &m_mass_);

		// TODO: colliding objects
	}

	Physics::GenericBounding Collider::GetBounding() const
	{
		return m_boundings_.Transform(GetWorldMatrix());
	}

	void Collider::UpdateInertiaTensor()
	{
		Quaternion rotation;

		if (m_type_ == BOUNDING_TYPE_BOX)
		{
			rotation = GetBounding<BoundingOrientedBox>().Orientation;
		}
		else if (m_type_ == BOUNDING_TYPE_SPHERE)
		{
			rotation = Quaternion::Identity;
		}

		Quaternion conjugate;

		rotation.Conjugate(conjugate);
		const Matrix invOrientation = Matrix::CreateFromQuaternion(conjugate);
		const Matrix orientation    = Matrix::CreateFromQuaternion(rotation);
		const Matrix inertiaScale   = Matrix::CreateScale(m_inverse_inertia_);

		const Matrix matrix = orientation * inertiaScale * invOrientation;

		XMStoreFloat3x3(&m_inertia_tensor_, matrix);
	}

	Collider::Collider(const WeakObjectBase& owner)
		: Component(COM_T_COLLIDER, owner),
		  m_type_(BOUNDING_TYPE_BOX),
		  m_boundings_(),
		  m_mass_(1.0f),
		  m_shape_meta_path_(),
		  m_inertia_tensor_(),
		  m_local_matrix_(Matrix::Identity) { }

	Collider::~Collider()
	{
#ifdef PHYSX_ENABLED
		CleanupPhysX();
#endif
	}

	void Collider::GenerateInertiaCube()
	{
		const Vector3 dim                = Vector3(GetBounding<BoundingOrientedBox>().Extents) * 2.f;
		const Vector3 dimensions_squared = dim * dim;

		m_inverse_inertia_.x = (12.0f * GetInverseMass()) /
		                       (dimensions_squared.y + dimensions_squared.z);
		m_inverse_inertia_.y = (12.0f * GetInverseMass()) /
		                       (dimensions_squared.x + dimensions_squared.z);
		m_inverse_inertia_.z = (12.0f * GetInverseMass()) /
		                       (dimensions_squared.x + dimensions_squared.y);
	}

	void Collider::GenerateInertiaSphere()
	{
		const float radius = GetBounding<BoundingSphere>().Radius;
		const float i      = 2.5f * GetInverseMass() / (radius * radius);

		m_inverse_inertia_ = Vector3(i, i, i);
	}

	void Collider::VerifyMaterial(boost::weak_ptr<Resources::Material> weak_material)
	{
		if (const auto& material = weak_material.lock())
		{
			if (const Strong<Resources::Shape> shape = material->GetResource<Resources::Shape>(0).lock())
			{
				SetShape(shape);
			}
		}
	}

	void Collider::UpdateByOwner(Weak<Component> component)
	{
		if (const auto& locked = component.lock())
		{
			if (locked->GetComponentType() == COM_T_RENDERER)
			{
				const Strong<Base::RenderComponent> render_component = locked->GetSharedPtr<Base::RenderComponent>();

				render_component->onMaterialChange.Listen(this, &Collider::VerifyMaterial);
				VerifyMaterial(render_component->GetMaterial());
			}
		}
	}

#ifdef PHYSX_ENABLED
	void Collider::UpdatePhysXShape()
	{
		const auto& scene = GetSceneManager().GetActiveScene().lock();

		// cleanup
		if (m_px_rb_static_)
		{
			if (scene)
			{
				scene->GetPhysXScene()->removeActor(*m_px_rb_static_);
			}

			if (m_px_rb_static_)
			{
				m_px_rb_static_->release();
				m_px_rb_static_ = nullptr;
			}
		}

		if (const auto& owner = GetOwner().lock())
		{
			Matrix world = GetWorldMatrix();

			Vector3 position, scale;
			Quaternion rotation;
			world.Decompose(scale, rotation, position);

			const physx::PxTransform px_transform(
				reinterpret_cast<const physx::PxVec3&>(position),
				reinterpret_cast<const physx::PxQuat&>(rotation));

			m_px_rb_static_ = GetPhysicsManager().GetPhysX()->createRigidDynamic(px_transform);
			//m_px_rb_static_->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
			m_px_rb_static_->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_GYROSCOPIC_FORCES, true);

			if constexpr (g_speculation_enabled)
			{
				m_px_rb_static_->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, true);
			}

			const auto& setupShape = [](const StrongObjectBase& in_owner, physx::PxShape* px_shape)
			{
				// todo: all ok for shape filtering
				physx::PxFilterData filter_data;
				filter_data.word0 = to_bitmask<physx::PxU32>(in_owner->GetLayer());
				filter_data.word1 = std::numeric_limits<unsigned long long>::max();

				px_shape->setSimulationFilterData(filter_data);
				px_shape->setContactOffset(0.01f);
				px_shape->setRestOffset(0.2f);
			};

			// assemble shape
			if (const auto& shape = m_shape_.lock())
			{
				for (const auto& mesh : shape->GetMeshes())
				{
					physx::PxTriangleMeshGeometry geo(mesh->GetPhysXMesh());
					geo.scale = reinterpret_cast<const physx::PxVec3&>(scale);

					if constexpr (g_debug)
					{
						const physx::PxVec3 px_scale = reinterpret_cast<const physx::PxVec3&>(scale);
						physx::PxVec3 px_scale_vec{ scale.x, scale.y, scale.z };
						_ASSERT(px_scale_vec == px_scale);
					}

					physx::PxShape* new_shape = physx::PxRigidActorExt::createExclusiveShape(*m_px_rb_static_, geo, *m_px_material_);
					setupShape(owner, new_shape);
				}
			}
			else
			{
				physx::PxTriangleMeshGeometry geo(m_type_ == BOUNDING_TYPE_BOX ? s_px_cube_stock_ : s_px_sphere_stock_);
				geo.scale = reinterpret_cast<const physx::PxVec3&>(scale);

				physx::PxShape* new_shape = physx::PxRigidActorExt::createExclusiveShape(*m_px_rb_static_, geo, *m_px_material_);
				setupShape(owner, new_shape);
			}

			// https://codebrowser.dev/qt6/qtquick3dphysics/src/3rdparty/PhysX/source/physxextensions/src/ExtDefaultSimulationFilterShader.cpp.html
			// due to the sequence of setting group, add shape first then update the actor group and, groups mask

			// todo: utilize group mask.
			physx::PxGroupsMask groups;
			std::memset(&groups.bits0, std::numeric_limits<uint16_t>::max(), sizeof(uint16_t) * 4);
			physx::PxSetGroupsMask(*m_px_rb_static_, groups);

			physx::PxSetGroup(*m_px_rb_static_, owner->GetLayer());
			m_px_rb_static_->userData = this;

			if (scene)
			{
				scene->GetPhysXScene()->addActor(*m_px_rb_static_);
			}

			m_previous_world_matrix_ = world;
			m_previous_scale_ = scale;
		}

		if (!m_px_rb_static_)
		{
			throw std::exception("Missing owner or transform component!");
			return;
		}
	}

	void Collider::CleanupPhysX()
	{
		if (m_px_rb_static_)
		{
			m_px_rb_static_->release();
			m_px_rb_static_ = nullptr;
		}

		if (m_px_material_)
		{
			m_px_material_->release();
			m_px_material_ = nullptr;
		}

		m_px_meshes_.clear();
	}
#endif

	void Collider::PreUpdate(const float& dt)
	{
		UpdateInertiaTensor();
	}

	void Collider::Update(const float& dt)
	{
		UpdateInertiaTensor();
	}

	void Collider::PostUpdate(const float& dt)
	{
		Component::PostUpdate(dt);

		if constexpr (g_debug)
		{
			if (m_collided_objects_.empty())
			{
				if (m_type_ == BOUNDING_TYPE_BOX)
				{
					GetDebugger().Draw(GetBounding<BoundingOrientedBox>(), Colors::OrangeRed);
				}
				else
				{
					GetDebugger().Draw(GetBounding<BoundingSphere>(), Colors::OrangeRed);
				}
			}
			else
			{
				if (m_type_ == BOUNDING_TYPE_BOX)
				{
					GetDebugger().Draw(GetBounding<BoundingOrientedBox>(), Colors::GreenYellow);
				}
				else
				{
					GetDebugger().Draw(GetBounding<BoundingSphere>(), Colors::GreenYellow);
				}
			}
		}
	}
} // namespace Engine::Component
