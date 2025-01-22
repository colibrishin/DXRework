#include "../Public/ModelRendererRenderTask.h"
#include "Source/Runtime/Components/RenderComponent/Public/egRenderComponent.h"
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.hpp"
#include "Source/Runtime/Core/Components/Transform/Public/Transform.h"
#include "Source/Runtime/Components/Animator/Public/Animator.h"
#include "Source/Runtime/Resources/BaseAnimation/Public/BaseAnimation.h"
#include "Source/Runtime/Resources/BoneAnimation/Public/BoneAnimation.h"
#include "Source/Runtime/Resources/AtlasAnimation/Public/AtlasAnimation.h"

namespace Engine 
{
    void ModelRendererRenderInstanceTask::Run(
            const Scene const* scene, 
            const RenderMapValueType* render_map, 
            const std::size_t map_size, 
            std::atomic<uint64_t>& instance_count) 
    {
        const auto& rendercomponents = scene->GetCachedComponents<Components::RenderComponent>();

        tbb::parallel_for_each(rendercomponents.begin(), rendercomponents.end(), [](const Weak<Components::RenderComponent>& comp)
        {
            if (const Strong<Components::RenderComponent>& raw_component = comp.lock())
            {
                // get model renderer, continue if it is disabled
                if (!raw_component->GetActive())
                {
                    return;
                }

                const Strong<Abstracts::ObjectBase>& obj = raw_component->GetOwner().lock();
                const Strong<Resources::Material> mtr = raw_component->GetMaterial().lock();
                const Strong<Components::Transform> tr  = obj->GetComponent<Components::Transform>().lock();

                // animator parameters
                float anim_frame    = 0.0f;
                UINT  anim_idx      = 0;
                UINT  anim_duration = 0;
                bool  no_anim       = false;
                UINT  atlas_x       = 0;
                UINT  atlas_y       = 0;
                UINT  atlas_w       = 0;
                UINT  atlas_h       = 0;

                if (const auto atr = obj->GetComponent<Components::Animator>().lock())
                {
                    anim_frame = atr->GetFrame();
                    anim_idx   = atr->GetAnimation();
                    no_anim    = !atr->GetActive();

                    if (const auto bone_anim = mtr->GetResource<Resources::BoneAnimation>(anim_idx).lock())
                    {
                        // Drop the fractional part and interpolate the frame in shader.
                        anim_duration = static_cast<UINT>(bone_anim->GetDuration());
                    }

                    if (const auto atlas_anim = mtr->GetResource<Resources::AtlasAnimation>(anim_idx).lock())
                    {
                        AtlasAnimationPrimitive::AtlasFramePrimitive atlas_frame{};
                        atlas_anim->GetFrame(anim_frame, atlas_frame);

                        atlas_x = atlas_frame.X;
                        atlas_y = atlas_frame.Y;
                        atlas_w = atlas_frame.Width;
                        atlas_h = atlas_frame.Height;
                    }
                }

                // Pre-mapping by the material.
                for (auto i = 0; i < SHADER_DOMAIN_MAX; ++i)
                {
                    const auto domain = static_cast<eShaderDomain>(i);

                    if (mtr->IsRenderDomain(domain))
                    {
                        RenderMapValueType& domain_map = out_map[domain];
                        RenderMap::accessor acc;

                        if (!domain_map.find(acc, RENDER_COM_T_MODEL))
                        {
                            domain_map.insert(acc, RENDER_COM_T_MODEL);
                        }

                        Graphics::SBs::InstanceModelSB sb{};
                        sb.SetWorld(tr->GetWorldMatrix().Transpose());
                        sb.SetFrame(anim_frame);
                        sb.SetAnimDuration(anim_duration);
                        sb.SetAnimIndex(anim_idx);
                        sb.SetNoAnim(no_anim);
                        sb.SetAtlasX(atlas_x);
                        sb.SetAtlasY(atlas_y);
                        sb.SetAtlasW(atlas_w);
                        sb.SetAtlasH(atlas_h);

                        // todo: stacking structured buffer data might be get large easily.
                        acc->second.push_back(std::make_tuple(obj, mtr, aligned_vector<Graphics::SBs::InstanceSB>{sb}));
                        instance_count.fetch_add(1);
                    }
                }
            }
        })
    }

    void ModelRendererInstanceTask::Cleanup(const RenderMapValueType* render_map)
    {
        render_map->clear();
    }
}