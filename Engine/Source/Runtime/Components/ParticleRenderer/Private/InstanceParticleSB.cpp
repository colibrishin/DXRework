#include "InstanceParticleSB.h"
#include "InstanceParticleSB.generated.h"

Engine::Graphics::SBs::InstanceParticleSB::InstanceParticleSB()
{
    SetLife(0.0f);
    SetActive(true);
    SetVelocity(Vector3::One);
    SetWorld(Matrix::Identity);
}

void Engine::Graphics::SBs::InstanceParticleSB::SetLife(const float life)
{
    SetParam(6, life);
}

void Engine::Graphics::SBs::InstanceParticleSB::SetActive(const bool active)
{
    SetParam(15, static_cast<int>(active));
}

void Engine::Graphics::SBs::InstanceParticleSB::SetVelocity(const Vector3& velocity)
{
    SetParam(3, velocity);
}

void Engine::Graphics::SBs::InstanceParticleSB::SetWorld(const Matrix& world)
{
    SetParam(0, world);
}

Engine::Matrix& Engine::Graphics::SBs::InstanceParticleSB::GetWorld()
{
    return GetParam<Matrix>(0);
}

bool& Engine::Graphics::SBs::InstanceParticleSB::GetActive()
{
    return reinterpret_cast<bool&>(GetParam<int>(15));
}
