#pragma once
#include "D3D12ComputePrimitiveShader.h"
#include "IGraphicAPI.h"

#include "D3D12GraphicResourcePrimitive.generated.h"

namespace Engine
{
    ECLASS( internal )
    struct D3D12GraphicResourcePrimitive : public IGraphicResource 
    {
        GENERATE_BODY

        ~D3D12GraphicResourcePrimitive() override;
        void Release() override;

    protected:
        void* GetResourceInternal() override;
        void** GetAddressOfInternal() override;

    private:
        ComPtr<ID3D12Resource> m_raw_resources_;
    };
}
