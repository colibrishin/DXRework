#include "pch.h"

#include "egMeshRenderer.h"
#include "egDebugger.hpp"
#include "egMesh.h"
#include "egTexture.h"
#include "egNormalMap.h"
#include "egResourceManager.hpp"
#include "egVertexShaderInternal.h"
#include "egShader.hpp"

SERIALIZER_ACCESS_IMPL(Engine::Components::MeshRenderer,
                       _ARTAG(_BSTSUPER(Component))
                       _ARTAG(m_resource_map_))

namespace Engine::Components
{
    MeshRenderer::MeshRenderer(const WeakObject& owner) : Component(COM_T_MESH_RENDERER, owner) {}

    void MeshRenderer::PreUpdate(const float& dt) {}

    void MeshRenderer::Update(const float& dt) {}

    void MeshRenderer::FixedUpdate(const float& dt) {}

    void MeshRenderer::PreRender(const float& dt) {}

    void MeshRenderer::PostRender(const float& dt) {}

    void MeshRenderer::Render(const float& dt)
    {
        Component::Render(dt);

        const auto mesh = m_mesh_.lock();

        if (!mesh)
        {
            GetDebugger().Log(L"MeshRenderer::Render() : Mesh is null");
            return;
        }

        if (m_vertex_shaders_.empty())
        {
            GetDebugger().Log(L"MeshRenderer::Render() : Vertex shader is null");
            return;
        }

        if (m_pixel_shaders_.empty())
        {
            GetDebugger().Log(L"MeshRenderer::Render() : Pixel shader is null");
            return;
        }

        const auto render_targets = m_mesh_.lock()->GetRemainingRenderIndex();

        auto vtx = m_vertex_shaders_.begin();
        auto pix = m_pixel_shaders_.begin();
        auto tex = m_textures_.begin();
        auto nrm = m_normal_maps_.begin();

        for (int i = 0; i < render_targets; ++i)
        {
            if (!m_textures_.empty()) tex->lock()->Render(dt);
            if (!m_normal_maps_.empty()) nrm->lock()->Render(dt);
            vtx->lock()->Render(dt);
            pix->lock()->Render(dt);
            mesh->Render(dt);

            GetRenderPipeline().UnbindResource(SR_NORMAL_MAP);
            GetRenderPipeline().UnbindResource(SR_TEXTURE);
            GetRenderPipeline().ResetShaders();

            CycleIterator(m_vertex_shaders_, vtx);
            CycleIterator(m_pixel_shaders_, pix);
            CycleIterator(m_textures_, tex);
            CycleIterator(m_normal_maps_, nrm);
        }

        mesh->ResetRenderIndex();
    }

    void MeshRenderer::OnDeserialized()
    {
        Component::OnDeserialized();

        for (const auto& [type, names] : m_resource_map_)
        {
            for (const auto& name : names)
            {
                if (type == RES_T_MESH)
                {
                    SetMesh(GetResourceManager().GetResource<Resources::Mesh>(name));
                }
                else if (type == RES_T_TEX)
                {
                    Add(GetResourceManager().GetResource<Resources::Texture>(name));
                }
                else if (type == RES_T_NORMAL)
                {
                    Add(GetResourceManager().GetResource<Resources::NormalMap>(name));
                }
                else if (type == RES_T_SHADER)
                {
                    // todo: need to recognize shader type

                    const auto prefix = name.substr(0, 2);

                    if (prefix == "vs") Add(GetResourceManager().GetResource<Graphic::VertexShader>(name));
                    else if (prefix == "ps") Add(GetResourceManager().GetResource<Graphic::PixelShader>(name));
                    else throw std::runtime_error("MeshRenderer::OnDeserialized() : Unknown shader type");
                }
                else
                {
                    throw std::runtime_error("MeshRenderer::OnDeserialized() : Unknown resource type");
                }
            }
        }
    }

    void MeshRenderer::SetMesh(const WeakMesh& mesh)
    {
        m_mesh_ = mesh;
        m_resources_.insert(mesh.lock());
        m_resource_map_[mesh.lock()->GetResourceType()].emplace(mesh.lock()->GetName());
    }

    const WeakMesh& MeshRenderer::GetMesh() const
    {
        return m_mesh_;
    }

    MeshRenderer::MeshRenderer() : Component(COM_T_MESH_RENDERER, {}) {}
}
