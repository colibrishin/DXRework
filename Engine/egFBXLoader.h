#pragma once
#include "egManager.hpp"
#include "egMesh.h"

namespace Engine::Manager
{
    class FBXLoader : public Abstract::Singleton<FBXLoader>
    {
    public:
        FBXLoader(SINGLETON_LOCK_TOKEN) : Abstract::Singleton<FBXLoader>() { }
        ~FBXLoader() override;
        void PreUpdate(const float& dt) override;
        void Update(const float& dt) override;
        void FixedUpdate(const float& dt) override;
        void PreRender(const float& dt) override;
        void Render(const float& dt) override;
        void PostRender(const float& dt) override;
        void Initialize() override;

        void LoadFBXFile(const std::filesystem::path& path, Resources::Mesh& target_mesh);
        //void LoadFBXFile(const std::filesystem::path& path, Engine::Resources::Animation& mesh);
        //void LoadFBXFile(const std::filesystem::path& path, std::vector<StrongTexture>& mesh);

    private:
        using IdxVtxPair = std::map<int, VertexElement*>;

        void IterateFBXMesh(FbxNode* child, Engine::Resources::Mesh& target_mesh);
        void IterateFBXSkeleton(FbxNode* node, UINT depth, Engine::Resources::Mesh& target_mesh);
        void IterateFBXSkin(FbxNode* child, Engine::Resources::Mesh& target_mesh);
        void RipDeformation(
            const FbxMesh * mesh, std::vector<Resources::Shape> & shape, const JointMap & joints);
        void RipVertexElementFromFBX(
            const FbxMesh* mesh, Resources::Shape& shape, Resources::IndexCollection& indices, int polygon_idx);

        FbxManager*  m_fbx_manager_  = nullptr;
        FbxScene*    m_fbx_scene_    = nullptr;
        FbxImporter* m_fbx_importer_ = nullptr;

    };
}
