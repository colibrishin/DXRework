#pragma once
#include <assimp/Importer.hpp>

#include "egDXCommon.h"
#include "egResource.h"
#include "egMesh.h"

namespace Engine::Resources
{
	class Model : public Abstract::Resource
	{
    public:
        INTERNAL_RES_CHECK_CONSTEXPR(RES_T_MODEL)

        Model(const std::filesystem::path& path);

        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;

        void RenderMeshOnly(const float& dt);

        BoundingBox                        GetBoundingBox() const;
        WeakMesh                           GetMesh(const std::string& name) const;
        WeakMesh                           GetMesh(const UINT index) const;
        const std::vector<const Vector3*>& GetVertices() const;

        UINT GetRenderIndex() const;
        UINT GetRemainingRenderIndex() const;
        void ResetRenderIndex();

        template <typename T, typename ResLock = std::enable_if_t<std::is_base_of_v<Resource, T>>>
        void Add(const boost::shared_ptr<T>& res)
        {
            if constexpr (which_resource<T>::value == RES_T_MESH)
            {
                m_meshes_.push_back(res);
            }
            else if constexpr (which_resource<T>::value == RES_T_TEX)
            {
                m_textures_.push_back(res);
            }
            else if constexpr (which_resource<T>::value == RES_T_NORMAL)
            {
                m_normal_maps_.push_back(res);
            }
            else if constexpr (which_resource<T>::value == RES_T_ANIM)
            {
                m_animations_.push_back(res);
            }
            else
            {
                static_assert("Invalid resource type");
            }

            UpdateVertices();
        }

        static StrongModel Create(const std::string& name, const std::vector<StrongResource>& resources);

        RESOURCE_SELF_INFER_GETTER(Model)
        RESOURCE_SELF_INFER_CREATE(Model)

    protected:
	    Model();

        void Load_INTERNAL() override;
        void Unload_INTERNAL() override;


	private:
        void UpdateVertices();

        UINT                         m_render_index_;
        std::vector<StrongMesh>      m_meshes_;
        StrongBone                   m_bone_;
        std::vector<StrongTexture>   m_textures_;
        std::vector<StrongNormalMap> m_normal_maps_;
        std::vector<StrongAnimation> m_animations_;
        BoundingBox                  m_bounding_box_;

        // non-serialized
        inline static Assimp::Importer s_importer_;
        std::vector<const Vector3*>    m_cached_vertices_;

        ComPtr<ID3D11ShaderResourceView> m_bone_srv_;
    };
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Model)