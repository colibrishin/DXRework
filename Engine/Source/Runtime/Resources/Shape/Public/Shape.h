#pragma once
#include <assimp/Importer.hpp>
#include <map>
#include "Source/Runtime/Core/Resource/Public/Resource.h"
#include "Source/Runtime/Resources/AnimationTexture/Public/AnimationTexture.h"
#include "Source/Runtime/Resources/Bone/Public/Bone.h"
#include "Source/Runtime/Resources/Mesh/Public/Mesh.h"

#include "Shape.generated.h"

namespace Engine::Resources
{
	ECLASS(resource, serialize)
	class ENGINE_SHAPE_API Shape : public Abstracts::Resource
	{
		GENERATE_BODY
	public:
		Shape(const std::filesystem::path& path);

		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostUpdate(const float dt) override;

#ifdef WITH_EDITOR
		void OnUIUpdate(UIContext* const parent, const float dt) override;
#endif

		void OnSerialized() override;
		void OnDeserialized() override;

		[[nodiscard]] BoundingBox                                GetBoundingBox() const;
		[[nodiscard]] Weak<Mesh>                                 GetMesh(const std::string& name) const;
		[[nodiscard]] Weak<Mesh>                                 GetMesh(UINT index) const;
		[[nodiscard]] Weak<AnimationTexture>                     GetAnimations() const;
		[[nodiscard]] std::vector<Strong<Mesh>>                  GetMeshes() const;
		[[nodiscard]] const std::vector<std::string>&            GetAnimationCatalog() const;
		[[nodiscard]] const std::map<UINT, BoundingOrientedBox>& GetBoneBoundingBoxes() const;

		template <typename T> requires (std::is_base_of_v<Resource, T> && !std::is_same_v<Resource, T>)
		void Add(const Weak<T>& res)
		{
			if (res.expired())
			{
				return;
			}

			const Strong<T>& locked = res.lock();
			
			if constexpr (Mesh::StaticIsBaseOf(T::StaticTypeHash()))
			{
				addMeshImpl(locked);
			}
			else if constexpr (Bone::StaticIsBaseOf(T::StaticTypeHash()))
			{
				addBoneImpl(locked);
			}
			else if constexpr (AnimationTexture::StaticIsBaseOf(T::StaticTypeHash()))
			{
				addAnimationImpl(locked);
			}
		}

		void Add(const Weak<Resource>& res);

	protected:
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		friend class Managers::Renderer;
		Shape();

		void addMeshImpl(const Strong<Mesh>& res);
		void addAnimationImpl(const Strong<AnimationTexture>& res);
		void addBoneImpl(const Strong<Bone>& res);

		void UpdateVertices();

		EPROPERTY()
		std::vector<std::string>     m_animation_catalog_;
		EPROPERTY()
		std::vector<MetadataPath> m_mesh_paths_;
		EPROPERTY()
		MetadataPath              m_bone_path_;
		EPROPERTY()
		MetadataPath              m_animations_path_;

		EPROPERTY()
		BoundingBox                         m_bounding_box_;
		EPROPERTY()
		std::map<UINT, BoundingOrientedBox> m_bone_bounding_boxes_;

		// non-serialized
		inline static Assimp::Importer s_importer_;
		std::vector<Strong<Mesh>>      m_meshes_;
		Strong<Bone>                   m_bone_;
		Strong<AnimationTexture>       m_animations_;

		bool m_ui_add_ui_opened_ = false;
		std::vector<VertexElement> m_cached_vertices_;
	};
}
