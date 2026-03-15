#include "D3D12GraphicResourcePrimitive.h"

namespace Engine
{
    D3D12GraphicResourcePrimitive::~D3D12GraphicResourcePrimitive()
    {
    }

    void D3D12GraphicResourcePrimitive::Release()
    {
        if ( m_raw_resources_ )
        {
            m_raw_resources_.Reset();
        }
    }
    void* D3D12GraphicResourcePrimitive::GetResourceInternal()
    {
        return m_raw_resources_.Get();
    }

    void** D3D12GraphicResourcePrimitive::GetAddressOfInternal()
    {
        return reinterpret_cast<void**>( m_raw_resources_.GetAddressOf() );
    }
}
