#pragma once
#include <assimp/Importer.hpp>

#include "egDXCommon.h"
#include "egResource.h"
#include "egMesh.h"

namespace Engine::Resources
{
	class Shape : public Abstract::Resource
	{
    public:
        INTERNAL_RES_CHECK_CONSTEXPR(RES_T_SHAPE)

        Shape(const std::filesystem::path& path);

        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;
        void PostUpdate(const float& dt) override;

        BoundingBox                        GetBoundingBox() const;
        WeakMesh                           GetMesh(const std::string& name) const;
        WeakMesh                           GetMesh(const UINT index) const;
        const std::vector<const Vector3*>& GetVertices() const;
        WeakBaseAnimation                  GetAnimation(const std::string& name) const;
        WeakBaseAnimation                  GetAnimation(const UINT index) const;

        UINT GetMeshCount() const;

        template <typename T, typename ResLock = std::enable_if_t<std::is_base_of_v<Resource, T>>>
        void Add(const boost::weak_ptr<T>& res)
        {
            if (res.expired()) return;

            if constexpr (which_resource<T>::value == RES_T_MESH)
            {
                m_meshes_.push_back(res.lock());
            }
            else if constexpr (which_resource<T>::value == RES_T_BONE)
            {
                m_bone_ = res.lock();
            }
            else if constexpr (which_resource<T>::value == RES_T_BONE_ANIM || 
                               which_resource<T>::value == RES_T_BASE_ANIM)
            {
                m_animations_.push_back(res.lock());
            }
            else
            {
                static_assert("Invalid resource type");
            }

            UpdateVertices();
        }

        std::vector<StrongMesh> GetMeshes() const;

        RESOURCE_SELF_INFER_GETTER(Shape)
        RESOURCE_SELF_INFER_CREATE(Shape)

    protected:
        SERIALIZER_ACCESS

        friend class Manager::Graphics::Renderer;
	    Shape();

        void Load_INTERNAL() override;
        void Unload_INTERNAL() override;


	private:
        void UpdateVertices();

        std::vector<StrongMesh>          m_meshes_;
        StrongBone                       m_bone_;
        std::vector<StrongBaseAnimation> m_animations_;
        BoundingBox                      m_bounding_box_;

        // non-serialized
        inline static Assimp::Importer s_importer_;
        std::vector<const Vector3*>    m_cached_vertices_;

        ComPtr<ID3D11ShaderResourceView> m_bone_srv_;
    };
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Shape)