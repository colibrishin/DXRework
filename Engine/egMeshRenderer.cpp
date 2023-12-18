#include "pch.h"

#include "egMeshRenderer.h"
#include "egDebugger.hpp"
#include "egMesh.h"
#include "egTexture.h"
#include "egNormalMap.h"
#include "egResourceManager.h"
#include "egVertexShaderInternal.h"
#include "egShader.hpp"

SERIALIZER_ACCESS_IMPL(Engine::Components::MeshRenderer,
                       _ARTAG(_BSTSUPER(Component))
                       _ARTAG(m_resource_map_))

namespace Engine::Components
{
    MeshRenderer::MeshRenderer(const WeakObject& owner) : Component(COMPONENT_PRIORITY_MESH_RENDERER, owner) {}

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
                if (type == typeid(Resources::Mesh).name())
                {
                    SetMesh(GetResourceManager().GetResource<Resources::Mesh>(name));
                }
                else if (type == typeid(Resources::Texture).name())
                {
                    AddTexture(GetResourceManager().GetResource<Resources::Texture>(name));
                }
                else if (type == typeid(Resources::NormalMap).name())
                {
                    AddNormalMap(GetResourceManager().GetResource<Resources::NormalMap>(name));
                }
                else if (type == typeid(Graphic::VertexShader).name())
                {
                    AddVertexShader(GetResourceManager().GetResource<Graphic::VertexShader>(name));
                }
                else if (type == typeid(Graphic::PixelShader).name())
                {
                    AddPixelShader(GetResourceManager().GetResource<Graphic::PixelShader>(name));
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
        m_resource_map_[mesh.lock()->GetVirtualTypeName()].emplace(mesh.lock()->GetName());
    }

    void MeshRenderer::AddTexture(const WeakTexture& texture)
    {
        m_textures_.emplace_back(texture);
        m_resources_.insert(texture.lock());
        m_resource_map_[texture.lock()->GetVirtualTypeName()].emplace(texture.lock()->GetName());
    }

    void MeshRenderer::AddNormalMap(const WeakNormalMap& normal_map)
    {
        m_normal_maps_.emplace_back(normal_map);
        m_resources_.insert(normal_map.lock());
        m_resource_map_[normal_map.lock()->GetVirtualTypeName()].emplace(normal_map.lock()->GetName());
    }

    void MeshRenderer::AddVertexShader(const WeakVertexShader& vertex_shader)
    {
        m_vertex_shaders_.emplace_back(vertex_shader);
        m_resources_.insert(vertex_shader.lock());
        m_resource_map_[vertex_shader.lock()->GetVirtualTypeName()].emplace(vertex_shader.lock()->GetName());
    }

    void MeshRenderer::AddPixelShader(const WeakPixelShader& pixel_shader)
    {
        m_pixel_shaders_.emplace_back(pixel_shader);
        m_resources_.insert(pixel_shader.lock());
        m_resource_map_[pixel_shader.lock()->GetVirtualTypeName()].emplace(pixel_shader.lock()->GetName());
    }

    void MeshRenderer::RemoveMesh()
    {
        m_resources_.erase(m_mesh_.lock());
        m_resource_map_.erase(m_mesh_.lock()->GetTypeName());
        m_resource_map_[m_mesh_.lock()->GetVirtualTypeName()].erase(m_mesh_.lock()->GetName());
        m_mesh_.reset();
    }

    void MeshRenderer::RemoveTexture(const WeakTexture& texture)
    {
        std::erase_if(m_textures_, [&texture](const auto& t) { return t.lock() == texture.lock(); });
        m_resources_.erase(texture.lock());
        m_resource_map_[texture.lock()->GetVirtualTypeName()].erase(texture.lock()->GetName());
    }

    void MeshRenderer::RemoveNormalMap(const WeakNormalMap& normal_map)
    {
        std::erase_if(m_normal_maps_, [&normal_map](const auto& n) { return n.lock() == normal_map.lock(); });
        m_resources_.erase(normal_map.lock());
        m_resource_map_[normal_map.lock()->GetVirtualTypeName()].erase(normal_map.lock()->GetName());
    }

    void MeshRenderer::RemoveVertexShader(const WeakVertexShader& vertex_shader)
    {
        std::erase_if(m_vertex_shaders_, [&vertex_shader](const auto& v) { return v.lock() == vertex_shader.lock(); });
        m_resources_.erase(vertex_shader.lock());
        m_resource_map_[vertex_shader.lock()->GetVirtualTypeName()].erase(vertex_shader.lock()->GetName());
    }

    void MeshRenderer::RemovePixelShader(const WeakPixelShader& pixel_shader)
    {
        std::erase_if(m_pixel_shaders_, [&pixel_shader](const auto& p) { return p.lock() == pixel_shader.lock(); });
        m_resources_.erase(pixel_shader.lock());
        m_resource_map_[pixel_shader.lock()->GetVirtualTypeName()].erase(pixel_shader.lock()->GetName());
    }

    const WeakMesh& MeshRenderer::GetMesh() const
    {
        return m_mesh_;
    }

    MeshRenderer::MeshRenderer() : Component(COM_T_MESH_RENDERER, {}) {}
}
