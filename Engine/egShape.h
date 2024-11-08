#pragma once
#include <assimp/Importer.hpp>

#include "egDXCommon.h"
#include "egMesh.h"
#include "egResource.h"

namespace Engine::Resources
{
	class Shape : public Abstract::Resource
	{
	public:
		RESOURCE_T(RES_T_SHAPE)

		Shape(const std::filesystem::path& path);

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostUpdate(const float& dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		BoundingBox                                GetBoundingBox() const;
		WeakMesh                                   GetMesh(const std::string& name) const;
		WeakMesh                                   GetMesh(UINT index) const;
		WeakAnimsTexture                           GetAnimations() const;
		const std::vector<VertexElement>&          GetVertices() const;
		std::vector<StrongMesh>                    GetMeshes() const;
		const std::vector<std::string>&            GetAnimationCatalog() const;
		const std::map<UINT, BoundingOrientedBox>& GetBoneBoundingBoxes() const;

		template <typename T, typename ResLock = std::enable_if_t<std::is_base_of_v<Resource, T>>>
		void Add(const boost::weak_ptr<T>& res)
		{
			if (res.expired())
			{
				return;
			}

			if constexpr (which_resource<T>::value == RES_T_MESH)
			{
				m_meshes_.push_back(res.lock());

				m_bounding_box_.Center  = Vector3::Zero;
				m_bounding_box_.Extents = Vector3::Zero;

				
				for (const auto& mesh : m_meshes_)
				{
					const BoundingOrientedBox& obb = mesh->GetBoundingBox();

					BoundingBox::CreateMerged(m_bounding_box_, m_bounding_box_, reinterpret_cast<const BoundingBox&>(obb));
				}

				m_mesh_paths_.push_back(res.lock()->GetMetadataPath().generic_string());
			}
			else if constexpr (which_resource<T>::value == RES_T_BONE)
			{
				m_bone_      = res.lock();
				m_bone_path_ = res.lock()->GetMetadataPath().generic_string();
			}
			else if constexpr (which_resource<T>::value == RES_T_ANIMS_TEX)
			{
				m_animations_      = res.lock();
				m_animations_path_ = res.lock()->GetMetadataPath().generic_string();
			}
			else
			{
				static_assert("Invalid resource type");
			}

			UpdateVertices();
		}

		RESOURCE_SELF_INFER_GETTER(Shape)
		RESOURCE_SELF_INFER_CREATE(Shape)

	protected:
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		SERIALIZE_DECL
		friend class Manager::Graphics::Renderer;
		Shape();

		void UpdateVertices();

		std::vector<std::string>     m_animation_catalog_;
		std::vector<MetadataPathStr> m_mesh_paths_;
		MetadataPathStr              m_bone_path_;
		MetadataPathStr              m_animations_path_;

		BoundingBox                         m_bounding_box_;
		std::map<UINT, BoundingOrientedBox> m_bone_bounding_boxes_;

		// non-serialized
		inline static Assimp::Importer s_importer_;
		std::vector<StrongMesh>        m_meshes_;
		StrongBone                     m_bone_;
		StrongAnimsTexture             m_animations_;

		std::vector<VertexElement> m_cached_vertices_;
	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Shape)
