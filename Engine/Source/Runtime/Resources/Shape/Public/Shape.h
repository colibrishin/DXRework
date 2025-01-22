#pragma once
#include <assimp/Importer.hpp>
#include <map>

#include "ModuleManager/Public/IModule.h"

#include "Source/Runtime/Core/Resource/Public/Resource.h"
#include "Source/Runtime/Resources/AnimationTexture/Public/AnimationTexture.h"
#include "Source/Runtime/Resources/Bone/Public/Bone.h"
#include "Source/Runtime/Resources/Mesh/Public/Mesh.h"

POLYMORPHIC_TYPE_MAP(Engine::Resources::Shape, Engine::Abstracts::Resource)

namespace Engine
{
	struct ShapeModule;
}

POLYMORPHIC_TYPE_MAP(Engine::ShapeModule, Engine::IModule);

namespace Engine
{
	struct ShapeModule : IModule
	{
		INLINE_COMPILE_TIME_TYPENAME(ShapeModule);

		void             Initialize() override;
		void             Shutdown() override;
		bool             DynamicLoadable() override;
	};
}

namespace Engine::Resources
{
	class ENGINE_SHAPE_API Shape : public Abstracts::Resource
	{
	public:
		INLINE_COMPILE_TIME_TYPENAME(Shape)

		Shape(const std::filesystem::path& path);

		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostUpdate(const float dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		[[nodiscard]] BoundingBox                                GetBoundingBox() const;
		[[nodiscard]] Weak<Mesh>                                 GetMesh(const std::string& name) const;
		[[nodiscard]] Weak<Mesh>                                 GetMesh(UINT index) const;
		[[nodiscard]] Weak<AnimationTexture>                     GetAnimations() const;
		[[nodiscard]] std::vector<Strong<Mesh>>                  GetMeshes() const;
		[[nodiscard]] const std::vector<std::string>&            GetAnimationCatalog() const;
		[[nodiscard]] const std::map<UINT, BoundingOrientedBox>& GetBoneBoundingBoxes() const;

		template <typename T, typename ResLock = std::enable_if_t<std::is_base_of_v<Resource, T>>>
		void Add(const Weak<T>& res)
		{
			if (res.expired())
			{
				return;
			}

			if constexpr (T::StaticTypeHash() == Mesh::StaticTypeHash())
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
			else if constexpr (T::StaticTypeHash() == Bone::StaticTypeHash())
			{
				m_bone_      = res.lock();
				m_bone_path_ = res.lock()->GetMetadataPath().generic_string();
			}
			else if constexpr (T::StaticTypeHash() == AnimationTexture::StaticTypeHash())
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

		RESOURCE_SELF_INFER_GETTER_DECL(Shape)
		RESOURCE_SELF_INFER_CREATE_DECL(Shape)

	protected:
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		friend class Managers::Renderer;
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
		std::vector<Strong<Mesh>>      m_meshes_;
		Strong<Bone>                   m_bone_;
		Strong<AnimationTexture>       m_animations_;

		std::vector<VertexElement> m_cached_vertices_;
	};
}
