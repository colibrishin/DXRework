#include "StructuredBuffer.h"
#include "InstanceModelSB.generated.h"

namespace Engine::Graphics::SBs
{
    ECLASS()
    struct ENGINE_MODELRENDERER_API InstanceModelSB : public InstanceSB
    {
        GENERATE_BODY
        
        InstanceModelSB();

        void SetFrame(const float frame);
        void SetWorld(const Matrix& world);
    };
}
