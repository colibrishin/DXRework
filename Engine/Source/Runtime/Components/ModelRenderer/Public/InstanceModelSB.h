#include "StructuredBuffer/Public/StructuredBuffer.h"
#include "InstanceModelSB.generated.h"

namespace Engine::Graphics::SBs
{
    ECLASS(serialize)
    struct ENGINE_MODELRENDERER_API InstanceModelSB : public InstanceSB
    {
        GENERATE_BODY
        
        InstanceModelSB();

        void SetFrame(const float frame);
        void SetAnimDuration(const UINT duration);
        void SetAnimIndex(const UINT index);
        void SetNoAnim(const bool no_anim);
        void SetAtlasX(const UINT x);
        void SetAtlasY(const UINT y);
        void SetAtlasW(const UINT w);
        void SetAtlasH(const UINT h);
        void SetRepeat(const bool repeat);
        void SetWorld(const Matrix& world);
    };
}
