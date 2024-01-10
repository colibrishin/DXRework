#include "pch.h"

#include "egBaseCollider.hpp"
#include "egBoundingGroup.hpp"
#include "egLayer.h"
#include "egObject.hpp"
#include "egTransform.h"
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

    Physics::GenericBounding bounding_getter::value(Abstract::Object& object)
    {
        const auto tr = object.GetComponent<Components::Transform>().lock();

        if (const auto cldr = object.GetComponent<Components::Collider>().lock())
        {
            auto bounding = cldr->GetBounding();
            bounding.Transform(tr->GetWorldMatrix());
            return bounding;
        }

        Physics::GenericBounding bounding;
        bounding.SetType(BOUNDING_TYPE_BOX);
        bounding.Transform(tr->GetWorldMatrix());
        return bounding;
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