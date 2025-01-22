#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <ranges>

#include "../Public/Shape.h"
#include "Shape.generated.h"

#include "Components/Collider/Public/Collider.h"

#include "ModuleManager/Public/ModuleManager.h"

#include "Source/Runtime/Core/VertexElement/Public/VertexElement.h"
#include "Source/Runtime/Resources/Mesh/Public/Mesh.h"
#include "Source/Runtime/Core/MathExtension/Public/MathExtension.hpp"
#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.h"
#include "Source/Runtime/Resources/Bone/Public/Bone.h"
#include "Source/Runtime/Resources/BoneAnimation/Public/BoneAnimation.h"
#include "Source/Runtime/Resources/AnimationTexture/Public/AnimationTexture.h"
#include "Source/Runtime/ShapeImporter/Public/ShapeImporter.h"

#include "UIHelpersResourceManager.h"

namespace Engine::Resources
{
	Shape::Shape(const std::filesystem::path& path)
		: Resource(path),
		  m_bounding_box_({}) {}

	void Shape::PreUpdate(const float dt) {}

	void Shape::Update(const float dt) {}

	void Shape::FixedUpdate(const float dt) {}

	void Shape::PostUpdate(const float dt) {}

#ifdef WITH_EDITOR
	void Shape::OnUIUpdate(UIContext* const parent, const float dt)
	{
		if (parent) 
		{
			Resource::OnUIUpdate(parent, dt);

			UIInterface& ui = UIInterfaceAccessor::GetInterface();
			*parent += ui.NewListBox({ "Mesh List", 0, 0 });
			*parent |= ui.NewDragAndDropTarget({ "RESOURCE", [&](void* ptr)
				{
					if (auto casted = static_cast<Strong<Resource>*>(ptr))
					{
						Add(*casted);
					}
				} });

			for (size_t i = 0; i < m_meshes_.size(); ++i)
			{
				const auto& mesh = m_meshes_[i].first;

				*parent |= ui.NewSelectable({ mesh->GetName(), mesh->m_ui_info_.dialogOpened});
				(*parent |= ui.NewButton({ "Edit Material" })).SetFunction([i, this]()
					{
						m_ui_material_add_opened_[i] = true;
					});
				*parent |= ui.NewSeparator({});

				if (mesh->m_ui_info_.dialogOpened)
				{
					if (UIContext context = ui.NewContext(ui.NewDialog({ mesh.get(), mesh->GetName(), mesh->m_ui_info_.dialogOpened })))
					{
						mesh->OnUIUpdate(&context, dt);
					}
				}
			}

			for (auto it = m_meshes_.begin(); it != m_meshes_.end(); ++it)
			{
				const auto& mesh = (*it).first;
				const size_t idx = std::distance(m_meshes_.begin(), it);

				if (m_ui_material_add_opened_[idx])
				{
					if (Weak<Resource> resources_to_load;
						UIHelpers::SingleResourceSelectionDialogInclusion<Shape, Material>(mesh->GetSharedPtr<Entity>(), resources_to_load))
					{
						if (const Strong<Resource>& res = resources_to_load.lock())
						{
							SetMaterial(idx, res->GetSharedPtr<Material>());
						}

						m_ui_material_add_opened_[std::distance(m_meshes_.begin(), it)] = false;
					}
				}
			}

			--*parent;

			(*parent |= ui.NewButton({ "Add New..." })).SetFunction([&]() 
				{
					m_ui_mesh_add_opened_ = !m_ui_mesh_add_opened_;
				});

			if (m_ui_mesh_add_opened_) 
			{
				if (std::vector<Weak<Resource>> resources_to_load;
					UIHelpers::MultipleResourceSelectionDialogInclusion<Shape, Mesh, AnimationTexture>(GetSharedPtr<Shape>(), resources_to_load))
				{
					for (const Weak<Resource>& resource : resources_to_load)
					{
						Add(resource);
					}

					m_ui_mesh_add_opened_ = false;
				}
			}
		}
	}
#endif

	void Shape::OnSerialized()
	{
		Resource::OnSerialized();
	}

	void Shape::OnDeserialized()
	{
		Resource::OnDeserialized();

		Load();
	}

	BoundingBox Shape::GetBoundingBox() const
	{
		return m_bounding_box_;
	}

	Weak<Mesh> Shape::GetMesh(const std::string& name) const
	{
		const auto it = std::ranges::find_if
				(
				 m_meshes_
				 , [&name](const auto& pair)
				 {
					 return pair.first->GetName() == name;
				 }
				);

		if (it != m_meshes_.end())
		{
			return it->first;
		}

		return {};
	}

	Weak<Mesh> Shape::GetMesh(const UINT index) const
	{
		if (m_meshes_.size() > index)
		{
			return m_meshes_[index].first;
		}

		return {};
	}

	Weak<Material> Shape::GetMaterial(UINT idx) const
	{
		if (m_meshes_.size() > idx)
		{
			return m_meshes_[idx].second;
		}

		return {};
	}

	Weak<AnimationTexture> Shape::GetAnimations() const
	{
		return m_animations_;
	}

	Weak<BaseAnimation> Shape::GetTransformAnimation() const
	{
		return m_tr_animation_;
	}

	const Shape::WeakMeshMaterialVector& Shape::GetMeshes() const
	{
		return m_cached_meshes_;
	}

	const std::vector<std::string>& Shape::GetAnimationCatalog() const
	{
		return m_animation_catalog_;
	}

	const std::map<UINT, BoundingOrientedBox>& Shape::GetBoneBoundingBoxes() const
	{
		return m_bone_bounding_boxes_;
	}

	void Shape::UpdateVertices()
	{
		m_cached_meshes_.clear();

		for (const auto& pair : m_meshes_)
		{
			m_cached_meshes_.push_back(pair);
		}
		
		m_cached_vertices_.clear();

		for (const auto& mesh : m_meshes_ | std::views::keys)
		{
			for (const auto& vertex : mesh->GetVertexCollection())
			{
				m_cached_vertices_.push_back(vertex);
			}
		}

		BoundingBox::CreateFromPoints(
			m_bounding_box_,
			m_cached_vertices_.size(),
			reinterpret_cast<const Vector3*>(m_cached_vertices_.data()),
			sizeof(VertexElement));

		std::map<UINT, std::vector<Vector3>> bone_vertices;

		for (const auto& vertex : m_cached_vertices_)
		{
			for (const auto& idx : vertex.boneElement.GetIndices())
			{
				const auto unique = std::ranges::find_if
				(
					bone_vertices[idx],
					[vertex](const Vector3& v)
					{
						return MathExtension::FloatCompare(v.x, vertex.position.x) &&
							MathExtension::FloatCompare(v.y, vertex.position.y) &&
							MathExtension::FloatCompare(v.z, vertex.position.z);
					}
				);

				if (unique != bone_vertices[idx].end())
				{
					continue;
				}

				bone_vertices[idx].push_back(vertex.position);
			}
		}

		for (const auto& [idx, vertices] : bone_vertices)
		{
			BoundingOrientedBox::CreateFromPoints
			(m_bone_bounding_boxes_[idx], vertices.size(), vertices.data(), sizeof(Vector3));
		}
	}

	void Shape::Add(const Weak<Resource>& res)
	{
		if (const Strong<Resource>& locked = res.lock())
		{
			if (locked->GetTypeHash()->IsDerivedOf(Mesh::StaticTypeHash()))
			{
				addMeshImpl(boost::reinterpret_pointer_cast<Mesh>(locked));
			}
			else if (locked->GetTypeHash()->IsDerivedOf(AnimationTexture::StaticTypeHash()))
			{
				addAnimationImpl(boost::reinterpret_pointer_cast<AnimationTexture>(locked));
			}
			else if (locked->GetTypeHash()->IsDerivedOf(BaseAnimation::StaticTypeHash()))
			{
				addTrAnimationImpl(boost::reinterpret_pointer_cast<BaseAnimation
				>(locked));
			}
		}
	}

	void Shape::SetMaterial(const Weak<Mesh>& target, const Weak<Material>& mat)
	{
		if (mat.expired() || target.expired()) 
		{
			return;
		}

		const Strong<Mesh>& mesh_locked = target.lock();

		const auto& it = std::ranges::find_if(m_meshes_, [&mesh_locked](const MeshMaterialPair<Strong>& pair)
			{
				return pair.first == mesh_locked;
			});

		if (it != m_meshes_.end())
		{
			SetMaterial(std::distance(m_meshes_.begin(), it), mat);
		}
	}

	void Shape::SetMaterial(const size_t target_mesh_idx, const Weak<Material>& mat)
	{
		if (m_meshes_.size() > target_mesh_idx)
		{
			return;
		}
		
		if (const Strong<Material>& locked = mat.lock())
		{
			m_meshes_[target_mesh_idx].second = locked;
			m_material_paths_[target_mesh_idx] = locked->GetMetadataPath();
			m_cached_meshes_[target_mesh_idx].second = locked;
		}
	}

	void Shape::Load_INTERNAL()
	{
		if (!GetMetadataPath().empty())
		{
			for (int i = 0; i < m_mesh_paths_.size(); ++i)
			{
				if (const auto mesh = Managers::ResourceManager::GetInstance().GetResourceByMetadataPath<Mesh>
						(m_mesh_paths_[i]).lock())
				{
					Add(mesh);
				}
			}

			if (const auto anims = Managers::ResourceManager::GetInstance().GetResourceByMetadataPath<AnimationTexture>
					(m_animations_path_).lock())
			{
				Add(anims);
			}

			return;
		}

		if (GetPath().empty())
		{
			return;
		}

		const auto scene = s_importer_.ReadFile
		(
			GetPath().string(),
			aiProcess_Triangulate | aiProcess_GenSmoothNormals |
			aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices |
			aiProcess_MakeLeftHanded | aiProcess_PopulateArmatureData
		);

		if (scene == nullptr)
		{
			throw std::runtime_error(s_importer_.GetErrorString());
		}

		aiMatrix4x4 axis = {
			1.f, 0.f, 0.f, 0.f,
			0.f, 0.f, -1.f, 0.f,
			0.f, -1.f, 0.f, 0.f,
			0.f, 0.f, 0.f, 1.f
		};

		scene->mRootNode->mTransformation = axis;

		if (scene->HasMeshes())
		{
			const unsigned shape_count = scene->mNumMeshes;
			Strong<Bone> generated_bone;

			for (unsigned i = 0; i < shape_count; ++i)
			{
				VertexCollection shape;
				IndexCollection  indices;

				const auto shape_ = scene->mMeshes[i];
				const auto mesh_name = shape_->mName.C_Str();

				const std::string mesh_lookup_name = GetName() + "_" + mesh_name + "_" + std::to_string(i);

				if (const auto mesh = Mesh::Get(mesh_lookup_name).lock())
				{
					m_meshes_.push_back({ mesh , {} });
					continue;
				}

				const unsigned v_count = shape_->mNumVertices;
				const unsigned f_count = shape_->mNumFaces;
				const unsigned b_count = shape_->mNumBones;

				if (f_count == 0)
				{
					continue;
				}

				// extract vertices
				for (unsigned j = 0; j < v_count; ++j)
				{
					auto vec = shape_->mVertices[j];
					vec *= axis;

					Vector2 tex_coord = { 0.f, 0.f };
					Vector3 normal_ = { 0.f, 0.f, 0.f };
					Vector3 tangent_ = { 0.f, 0.f, 0.f };
					Vector3 binormal_ = { 0.f, 0.f, 0.f };
					Vector4 color = { 1.f, 0.f, 0.f, 1.f };

					if (shape_->HasTextureCoords(0))
					{
						const auto tex = shape_->mTextureCoords[0]; // Assuming UV exists in 2D
						tex_coord = Vector2{ tex[j].x, tex[j].y };
					}

					if (shape_->HasNormals())
					{
						auto normal = shape_->mNormals[j];
						normal *= axis;
						normal_ = Vector3{ normal.x, normal.y, normal.z };
					}

					if (shape_->HasTangentsAndBitangents())
					{
						auto tangent = shape_->mTangents[j];
						auto binormal = shape_->mBitangents[j];

						tangent *= axis;
						binormal *= axis;

						tangent_ = Vector3{ tangent.x, tangent.y, tangent.z };
						binormal_ = Vector3{ binormal.x, binormal.y, binormal.z };
					}

					if (shape_->HasVertexColors(0))
					{
						const auto& col = shape_->mColors[0];
						color = Vector4{ col[j].r, col[j].g, col[j].b, col[j].a };
					}

					const auto vtx = VertexElement
					(
						{vec.x, vec.y, vec.z},
						color,
						tex_coord,
						normal_,
						tangent_,
						binormal_,
						{}
					);

					shape.emplace_back(vtx);
				}

				for (unsigned j = 0; j < f_count; ++j)
				{
					const auto face = shape_->mFaces[j];
					const auto indices_ = face.mNumIndices;

					// extract indices
					for (unsigned k = 0; k < indices_; ++k)
					{
						indices.push_back(face.mIndices[k]);
					}
				}

				if (shape_->HasBones())
				{
					Graphics::BonePrimitiveMap bone_map;

					for (unsigned j = 0; j < b_count; ++j)
					{
						const auto bone = shape_->mBones[j];
						auto       offset = bone->mOffsetMatrix;
						auto       transformation = bone->mNode->mTransformation;

						offset *= axis;
						transformation *= axis;

						const std::string bone_name = bone->mName.C_Str();

						if (const auto check = Managers::ResourceManager::GetInstance().GetResource<Bone>
							(mesh_lookup_name + "_BONE").lock())
						{
							generated_bone = check;
							continue;
						}

						const unsigned weight_count = bone->mNumWeights;
						// todo: expand
						for (unsigned influence = 0; influence < weight_count; ++influence)
						{
							const auto weight = bone->mWeights[influence];
							const auto vertex_id = weight.mVertexId;
							const auto weight_ = weight.mWeight;
							auto& vtx_bone = shape[vertex_id].boneElement;

							vtx_bone.Append(j, weight_);
						}

						const auto parent = bone->mNode->mParent;
						const auto parent_name = parent->mName.C_Str();
						int        parent_idx = -1;

						if (bone_map.contains(parent_name))
						{
							parent_idx = bone_map[parent_name].GetIndex();
						}

						BonePrimitive bone_info;
						bone_info.SetIndex(j);
						bone_info.SetParentIndex(parent_idx);
						bone_info.SetInvBindPose(ShapeImporter::AiMatrixToDirectXTranspose(offset));
						bone_info.SetTransform(ShapeImporter::AiMatrixToDirectXTranspose(transformation));

						bone_map[bone_name] = bone_info;
					}

					// create and forget
					generated_bone = Bone::Create(mesh_lookup_name + "_BONE", bone_map);
				}

				const Strong<Mesh>& mesh = Mesh::Create(mesh_lookup_name, shape, indices);
				Add(mesh);
			}

			if (scene->HasAnimations())
			{
				std::vector<Strong<BoneAnimation>> animations;
				const unsigned                     animation_count = scene->mNumAnimations;

				for (unsigned j = 0; j < animation_count; ++j)
				{
					const auto        animation_ = scene->mAnimations[j];
					const unsigned    affect_bone_count = animation_->mNumChannels;
					const std::string anim_name = animation_->mName.C_Str();

					if (const auto check = Managers::ResourceManager::GetInstance().GetResource<BoneAnimation>(anim_name + "_ANIM").lock())
					{
						m_animation_catalog_.push_back(anim_name + "_ANIM");
						continue;
					}

					const auto duration = animation_->mDuration;
					const auto ticks_per_second = animation_->mTicksPerSecond == 0 ? 25.f : animation_->mTicksPerSecond;

					AnimationPrimitive animation
					(
						anim_name, static_cast<float>(duration), static_cast<float>(ticks_per_second),
						ShapeImporter::AiMatrixToDirectXTranspose
						(
							scene->mRootNode->mTransformation.
							Inverse()
						)
					);

					for (unsigned k = 0; k < affect_bone_count; ++k)
					{
						const auto channel = animation_->mChannels[k];
						const auto bone_name = channel->mNodeName;

						const auto bone = generated_bone->GetBone(bone_name.C_Str());

						if (!bone)
						{
							// todo: recover? multiplying inv_bind_pose * global_transform * global_inverse_transform
							// might be enough to get the final transform
							continue;
						}

						BoneAnimationPrimitive bone_animation;
						bone_animation.SetIndex(bone->GetIndex());

						const unsigned positions = channel->mNumPositionKeys;
						for (unsigned l = 0; l < positions; ++l)
						{
							const auto key = channel->mPositionKeys[l];
							const auto time = static_cast<float>(key.mTime);
							const auto value = key.mValue;

							bone_animation.AddPosition(time, Vector3{ value.x, value.y, value.z });
						}

						const unsigned rotations = channel->mNumRotationKeys;
						for (unsigned l = 0; l < rotations; ++l)
						{
							const auto key = channel->mRotationKeys[l];
							const auto time = static_cast<float>(key.mTime);
							const auto value = key.mValue;

							bone_animation.AddRotation(time, Quaternion{ value.x, value.y, value.z, value.w });
						}

						const unsigned scalings = channel->mNumScalingKeys;
						for (unsigned l = 0; l < scalings; ++l)
						{
							const auto key = channel->mScalingKeys[l];
							const auto time = static_cast<float>(key.mTime);
							const auto value = key.mValue;

							bone_animation.AddScale(time, Vector3{ value.x, value.y, value.z });
						}

						animation.Add(bone_name.C_Str(), bone_animation);
					}

					// todo: need an uuid to mark the supported bone.
					const Strong<BoneAnimation>& anim = BoneAnimation::Create(anim_name + "_ANIM", animation);
					m_animation_catalog_.push_back(anim_name + "_ANIM");
					animations.push_back(anim);
				}

				for (const auto& anim : animations)
				{
					Managers::ResourceManager::GetInstance().AddResource(anim);
				}

				const Strong<AnimationTexture>& anims = AnimationTexture::Create(GetName() + "_ANIMS", animations);
				Add(anims);
			}

			UpdateVertices();
			
			//m_bounding_box_.Transform(m_bounding_box_, AiMatrixToDirectXTranspose(scene->mRootNode->mTransformation));
		}
		else
		{
			throw std::runtime_error("No meshes found in file");
		}
	}

	void Shape::Unload_INTERNAL()
	{
		m_meshes_.clear();
		m_animation_catalog_.clear();
		m_bone_bounding_boxes_.clear();
		m_cached_vertices_.clear();
		m_bounding_box_ = {};
	}

	Shape::Shape()
		: Resource(""),
		  m_bounding_box_({}) {}

	void Shape::addMeshImpl(const Strong<Mesh>& res)
	{
		m_meshes_.push_back({ res, {} });

#if WITH_EDITOR
		m_ui_material_add_opened_.push_back(false);
#endif
		m_bounding_box_.Center  = {0, 0, 0};
		m_bounding_box_.Extents = {0, 0, 0};
				
		for (const auto& mesh : m_meshes_ | std::views::keys)
		{
			const BoundingOrientedBox& obb = mesh->GetBoundingBox();
			BoundingBox::CreateMerged(m_bounding_box_, m_bounding_box_, reinterpret_cast<const BoundingBox&>(obb));
		}
		
		m_mesh_paths_.push_back(res->GetMetadataPath().generic_string());
		UpdateVertices();
	}
	
	void Shape::addAnimationImpl(const Strong<AnimationTexture>& res)
	{
		m_animations_      = res;
		m_animations_path_ = res->GetMetadataPath().generic_string();
		m_animation_catalog_.clear();
		m_animation_catalog_.reserve(m_animations_->GetAnimations().size());

		for (const auto& animation : m_animations_->GetAnimations())
		{
			if (const Strong<Resources::BoneAnimation>& locked = animation.lock())
			{
				m_animation_catalog_.push_back(locked->GetName());
			}
		}
		
		// Sorts animations by name for consistency of the index of animation.
		std::ranges::sort(m_animation_catalog_, [](const std::string& lhs, const std::string& rhs){return lhs < rhs;});
	}
	void Shape::addTrAnimationImpl(const Strong<BaseAnimation>& res)
	{
		m_tr_animation_ = res;
		m_tr_animation_path_ = res->GetMetadataPath();
	}
}
