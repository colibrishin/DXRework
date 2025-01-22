#pragma once
#include <assimp/Importer.hpp>
#include <map>

#include "BaseAnimation.h"

#include "Source/Runtime/Core/Resource/Public/Resource.h"
#include "Source/Runtime/Resources/AnimationTexture/Public/AnimationTexture.h"
#include "Source/Runtime/Resources/Bone/Public/Bone.h"
#include "Source/Runtime/Resources/Mesh/Public/Mesh.h"
#include "Source/Runtime/Resources/Material/Public/Material.h"

#include "Shape.generated.h"

namespace Engine::Resources
{
	ECLASS(resource, serialize)
	class ENGINE_SHAPE_API Shape : public Abstracts::Resource
	{
		GENERATE_BODY
	public:

		template <template <typename> typename T>
		using MeshMaterialPair = std::pair<T<Mesh>, T<Material>>;

		typedef std::vector<MeshMaterialPair<Weak>> WeakMeshMaterialVector;
		typedef std::vector<MeshMaterialPair<Strong>> StrongMeshMaterialVector;

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
		[[nodiscard]] Weak<Material>                             GetMaterial(UINT idx) const;
		[[nodiscard]] Weak<AnimationTexture>                     GetAnimations() const;
		[[nodiscard]] Weak<BaseAnimation>                        GetTransformAnimation() const;
		[[nodiscard]] const WeakMeshMaterialVector&              GetMeshes() const;
		[[nodiscard]] const std::vector<std::string>&            GetAnimationCatalog() const;
		[[nodiscard]] const std::map<UINT, BoundingOrientedBox>& GetBoneBoundingBoxes() const;

		template <typename T> requires (std::is_base_of_v<Resource, T> && !std::is_same_v<Resource, T>)
		void Add(const Weak<T>& res)
		{
			if (const Strong<T>& locked = res.lock())
			{
				if constexpr (T::StaticIsDerivedOf(Mesh::StaticTypeHash()))
				{
					addMeshImpl(locked);
				}
				else if constexpr (T::StaticIsDerivedOf(AnimationTexture::StaticTypeHash()))
				{
					addAnimationImpl(locked);
				}
				else if constexpr (T::StaticIsDerivedOf(BaseAnimation::StaticTypeHash()))
				{
					addTrAnimationImpl(locked);
				}
			}
		}

		void Add(const Weak<Resource>& res);

		void SetMaterial(const Weak<Mesh>& target, const Weak<Material>& mat);
		void SetMaterial(const size_t target_mesh_idx, const Weak<Material>& mat);
		
	protected:
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		friend class Managers::Renderer;
		Shape();

		void addMeshImpl(const Strong<Mesh>& res, const bool add_path = true);
		void addAnimationImpl(const Strong<AnimationTexture>& res);
		void addTrAnimationImpl(const Strong<BaseAnimation>& res);

		void UpdateVertices();

		EPROPERTY()
		std::vector<std::string> m_animation_catalog_;
		EPROPERTY()
		std::vector<MetadataPath> m_mesh_paths_;
		EPROPERTY()
		std::vector<MetadataPath> m_material_paths_;
		EPROPERTY()
		MetadataPath m_animations_path_;
		EPROPERTY()
		MetadataPath m_tr_animation_path_;

		EPROPERTY()
		BoundingBox m_bounding_box_;
		EPROPERTY()
		std::map<UINT, BoundingOrientedBox> m_bone_bounding_boxes_;

#if WITH_EDITOR
		bool m_ui_mesh_add_opened_ = false;
		std::vector<bool> m_ui_material_add_opened_{};
#endif

		// non-serialized
		inline static Assimp::Importer s_importer_;
		
		StrongMeshMaterialVector m_meshes_;

		WeakMeshMaterialVector m_cached_meshes_;

		Strong<AnimationTexture> m_animations_;

		Strong<BaseAnimation> m_tr_animation_;

		aligned_vector<VertexElement> m_cached_vertices_;
	};
}
