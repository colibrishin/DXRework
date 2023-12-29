#include "pch.h"
#include "egLayer.h"
#include "egObject.hpp"
#include "egResource.h"
#include "egScene.hpp"

namespace Engine
{
    bool ResourcePriorityComparer::operator()(
        const StrongResource Left,
        const StrongResource Right) const
    {
        if (Left->GetResourceType() != Right->GetResourceType())
        {
            return Left->GetResourceType() < Right->GetResourceType();
        }

        return Left->GetID() < Right->GetID();
    }

    bool ComponentPriorityComparer::operator()(
        const WeakComponent Left,
        const WeakComponent Right) const
    {
        if (Left.lock()->GetComponentType() != Right.lock()->GetComponentType())
        {
            return Left.lock()->GetComponentType() < Right.lock()->GetComponentType();
        }

        return Left.lock()->GetID() < Right.lock()->GetID();
    }
} // namespace Engine


namespace DX
{
    const char* com_exception::what() const noexcept
    {
        static char s_str[64] = {};
        sprintf_s(
                  s_str, "Failure with HRESULT of %08X",
                  static_cast<unsigned int>(result));
        return s_str;
    }

    void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw com_exception(hr);
        }
    }
}

namespace FMOD::DX
{
    const char* fmod_exception::what() const noexcept
    {
        static char s_str[64] = {};
            sprintf_s(
                      s_str, "Failure with FMOD_RESULT of %08X",
                      static_cast<unsigned int>(result));
            return s_str;
    }

    void ThrowIfFailed(FMOD_RESULT hr)
    {
        if (hr != FMOD_OK)
        {
            throw fmod_exception(hr);
        }
    }
}