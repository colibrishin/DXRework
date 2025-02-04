#include "D3D12GraphicResourcePrimitive.h"
#include "D3D12GraphicResourcePrimitive.generated.h"

namespace Engine
{
    void D3D12GraphicResourcePrimitive::SetResource(void* resource)
    {
        GraphicResourcePrimitive::SetResource( resource );
        m_raw_resources_ = static_cast<ID3D12Resource*>(resource);
    }

    void D3D12GraphicResourcePrimitive::Release()
    {
        if (m_raw_resources_)
        {
            m_raw_resources_->Release();
            SetResource(nullptr);   
        }
    }
}
