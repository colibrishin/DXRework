#include "../Public/ModelRendererRenderTask.h"
#include "ModelRendererRenderTask.generated.h"
#include <tbb/parallel_for_each.h>
#include <tbb/concurrent_vector.h>

#include "InstanceModelSB.h"
#include "ModelRenderer.h"
#include "RenderPipeline.h"
#include "Renderer.h"

#include "Source/Runtime/Core/ModuleManager/Public/ModuleManager.h"
#include "Source/Runtime/Components/RenderComponent/Public/egRenderComponent.h"
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.h"
#include "Source/Runtime/Core/Components/Transform/Public/Transform.h"
#include "Source/Runtime/Components/Animator/Public/Animator.h"
#include "Source/Runtime/Resources/AtlasAnimationTexture/Public/AtlasAnimationTexture.h"
#include "Source/Runtime/Resources/Shape/Public/Shape.h"

MODULE_IMPL(Engine::ModelRendererRenderInstanceTaskModule, ModelRendererRenderInstanceTask)

namespace Engine 
{
	void ModelRendererRenderInstanceTaskModule::Initialize()
	{
		Managers::Renderer::GetInstance().RegisterRenderInstance(L"ModelRendererRenderInstanceTask", new ModelRendererRenderInstanceTask());
	}

	void ModelRendererRenderInstanceTaskModule::Shutdown()
	{
        Managers::Renderer::GetInstance().UnregisterRenderInstance(L"ModelRendererRenderInstanceTask");
	}

	bool ModelRendererRenderInstanceTaskModule::DynamicLoadable()
	{
		return true;
	}

    ModelRendererRenderInstanceTask::ModelRendererRenderInstanceTask()
        : m_instance_ticket_(SingletonSpinLock::GetInstance().Register()) {}

    void ModelRendererRenderInstanceTask::Run(
            Scene const* scene, 
            RenderMap* render_map,
            const size_t map_size) 
    {
        const auto& mrs = scene->GetCachedComponentsConcurrent<Components::ModelRenderer>();

        tbb::parallel_for_each(mrs.begin(), mrs.end(), [&](const Weak<Abstracts::Component>& comp)
        {
            if (const Strong<Abstracts::Component>& raw_component = comp.lock())
            {
                // get model renderer, continue if it is disabled
                if (!raw_component->GetActive())
                {
                    return;
                }

                const Strong<Components::ModelRenderer>& mr = raw_component->GetSharedPtr<Components::ModelRenderer>();
                const Strong<Abstracts::ObjectBase>& obj = raw_component->GetOwner().lock();
                const Strong<Resources::Shape> shape = mr->GetShape().lock();
                const Strong<Components::Transform> tr  = obj->GetComponent<Components::Transform>().lock();

                // Pre-mapping by the shader domain.
                for (size_t i = 0; i < map_size; ++i)
                {
                    const auto domain = static_cast<eShaderDomain>(i);
                    auto& domain_map = render_map[domain];

                    RenderMap::accessor acc;
                    if (!domain_map.find(acc, Components::ModelRenderer::StaticTypeHash()))
                    {
                        domain_map.insert(acc, Components::ModelRenderer::StaticTypeHash());
                    }
                    
                    for (const auto& [mesh, mtr] : shape->GetMeshes())
                    {
                        if (mesh.expired() || mtr.expired())
                        {
                            continue;
                        }

                        const Strong<Resources::Material>& locked_mtr = mtr.lock();
                        const Strong<Resources::Mesh>& locked_mesh = mesh.lock();

                        MeshMap::accessor mesh_acc;
                        if (!acc->second.find(mesh_acc, locked_mesh))
                        {
                            acc->second.insert(mesh_acc, locked_mesh);
                        }
                        
                        if (const Strong<Resources::Shader>& locked_shader = locked_mtr->GetShader().lock())
                        {
                            if (locked_shader->GetDomain() != domain)
                            {
                                continue;
                            }

                            decltype(mesh_acc->second)::accessor shader_acc;
                            if (!mesh_acc->second.find(shader_acc, locked_shader))
                            {
                                mesh_acc->second.insert(shader_acc, locked_shader);
                            }

                            InstancePair instance_pair;
                            instance_pair.object = obj;
                            
                            instance_pair.instance = GetInstance();
                            instance_pair.instance->SetParam(0, tr->GetWorldMatrix().Transpose());
                            
                            // Copy the material primitive and animator primitive if it exists.
                            locked_mtr->GetPrimitive().Apply(*instance_pair.instance);

                            if (const Strong<Components::Animator>& anim = obj->GetComponent<Components::Animator>().lock())
                            {
                                anim->GetPrimitive().Apply(*instance_pair.instance);
                            }

                            std::ranges::copy(
                                locked_mtr->GetTextures().begin(),
                                locked_mtr->GetTextures().end(),
                                instance_pair.textures.begin());

                            if (const Strong<Resources::AnimationTexture>& anims = shape->GetAnimations().lock())
                            {
                                instance_pair.reservedTextures[RESERVED_USER_TEX_BONES - RESERVED_USER_TEX_BEGIN] = anims;
                            }
                            
                            if (const Strong<Resources::AtlasAnimationTexture>& atlas = locked_mtr->GetAtlasTexture().lock())
                            {
                                instance_pair.reservedTextures[RESERVED_USER_TEX_ATLAS - RESERVED_USER_TEX_BEGIN] = atlas;
                            }
                            
                            shader_acc->second.emplace_back(instance_pair);
                        }
                    }
                }
            }
        });
    }

    void ModelRendererRenderInstanceTask::Cleanup(RenderMap* render_map, const size_t map_size)
    {
        for (size_t i = 0; i < map_size; ++i)
        {
            auto& domain_map = render_map[i];

            if (RenderMap::accessor acc;
                domain_map.find(acc, Components::ModelRenderer::StaticTypeHash()))
            {
                domain_map.erase(Components::ModelRenderer::StaticTypeHash());
            }
        }

	    for (Graphics::SBs::InstanceSB* instance : m_instance_generated_)
        {
            m_instance_allocator_.deallocate(instance);
	        instance = nullptr;
        }
    }

    Graphics::SBs::InstanceSB* ModelRendererRenderInstanceTask::GetInstance()
	{
	    SpinLockToken token = SingletonSpinLock::GetInstance().Lock(m_instance_ticket_);
	    Graphics::SBs::InstanceSB* generated = m_instance_allocator_.allocate();
	    const auto& it = std::ranges::find_if(m_instance_generated_, [&](const Graphics::SBs::InstanceSB* ptr)
        {
            return ptr == nullptr;
        });

	    if (it == m_instance_generated_.end())
	    {
	        m_instance_generated_.push_back(generated);
	        return generated;
	    }

	    *it = generated;
        return generated;
    }
}
