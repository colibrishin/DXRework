#include "InstanceModelSB.h"
#include "InstanceModelSB.generated.h"

Engine::Graphics::SBs::InstanceModelSB::InstanceModelSB()
{
    SetFrame(0.f);
    SetWorld(Matrix::Identity);
}

void Engine::Graphics::SBs::InstanceModelSB::SetFrame(const float frame)
{
    SetParam(0, frame);
}

void Engine::Graphics::SBs::InstanceModelSB::SetWorld(const Matrix& world)
{
    SetParam(0, world);
}
