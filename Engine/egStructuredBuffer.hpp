#pragma once
#include "Windows.h"

namespace Engine::Graphics
{
	template <typename T>
    class StructuredBuffer
	{
    public:
		StructuredBuffer();
        ~StructuredBuffer() = default;

		void            Create(UINT size, const T* initial_data, bool is_mutable = false);
		void __fastcall SetData(UINT size, const T* src_ptr);
		void __fastcall GetData(UINT size, T* dst_ptr);
		void            Bind(const eShaderType shader);
        void            Unbind(const eShaderType shader);
        void            Clear();

    private:
        void InitializeSRV(UINT size);
        void InitializeMainBuffer(UINT size, const T* initial_data);
        void InitializeWriteBuffer(UINT size);
        void InitializeReadBuffer(UINT size);

        bool m_b_mutable_;
	    UINT m_size_;

		ComPtr<ID3D11Buffer> m_buffer_;
		ComPtr<ID3D11ShaderResourceView> m_srv_;

		ComPtr<ID3D11Buffer> m_write_buffer_;
        ComPtr<ID3D11Buffer> m_read_buffer_;

	};

    template <typename T>
    void StructuredBuffer<T>::InitializeSRV(UINT size)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
        srv_desc.Format               = DXGI_FORMAT_UNKNOWN;
        srv_desc.ViewDimension        = D3D11_SRV_DIMENSION_BUFFER;
        srv_desc.Buffer.ElementOffset = 0;
        srv_desc.Buffer.ElementWidth  = size;

        DX::ThrowIfFailed(GetD3Device().GetDevice()->CreateShaderResourceView(m_buffer_.Get(), &srv_desc, m_srv_.ReleaseAndGetAddressOf()));
    }

    template <typename T>
    void StructuredBuffer<T>::InitializeMainBuffer(UINT size, const T* initial_data)
    {
        if ((sizeof(T) * size) % 16 != 0)
        {
            throw std::runtime_error("StructuredBuffer size need to be dividable by 16");
        }

        D3D11_BUFFER_DESC desc;

        desc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
        desc.ByteWidth           = sizeof(T) * size;
        desc.CPUAccessFlags      = 0;
        desc.StructureByteStride = sizeof(T);
        desc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        desc.Usage               = D3D11_USAGE_DEFAULT;

        if (initial_data != nullptr)
        {
            D3D11_SUBRESOURCE_DATA data;
            data.pSysMem = initial_data;
            DX::ThrowIfFailed(GetD3Device().GetDevice()->CreateBuffer(&desc, &data, m_buffer_.ReleaseAndGetAddressOf()));
        }
        else
        {
            DX::ThrowIfFailed(GetD3Device().GetDevice()->CreateBuffer(&desc, nullptr, m_buffer_.ReleaseAndGetAddressOf()));
        }
    }

    template <typename T>
    void StructuredBuffer<T>::InitializeWriteBuffer(UINT size)
    {
        D3D11_BUFFER_DESC w_desc;
        w_desc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
        w_desc.ByteWidth           = sizeof(T) * size;
        w_desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
        w_desc.StructureByteStride = sizeof(T);
        w_desc.MiscFlags           = 0;
        w_desc.Usage               = D3D11_USAGE_DYNAMIC;

        DX::ThrowIfFailed(GetD3Device().GetDevice()->CreateBuffer(&w_desc, nullptr, m_write_buffer_.ReleaseAndGetAddressOf()));
    }

    template <typename T>
    StructuredBuffer<T>::StructuredBuffer()
    : m_b_mutable_(false),
      m_size_(0)
    {
        static_assert(sizeof(T) <= 2048, "StructuredBuffer struct T size is too big");
    }

    template <typename T>
    void StructuredBuffer<T>::InitializeReadBuffer(UINT size)
    {
        D3D11_BUFFER_DESC r_desc;
        r_desc.BindFlags           = 0;
        r_desc.ByteWidth           = sizeof(T) * size;
        r_desc.CPUAccessFlags      = D3D11_CPU_ACCESS_READ;
        r_desc.StructureByteStride = sizeof(T);
        r_desc.MiscFlags           = 0;
        r_desc.Usage               = D3D11_USAGE_STAGING;

        DX::ThrowIfFailed(GetD3Device().GetDevice()->CreateBuffer(&r_desc, nullptr, m_read_buffer_.ReleaseAndGetAddressOf()));
    }

    template <typename T>
    void StructuredBuffer<T>::Create(UINT size, const T* initial_data, bool is_mutable)
    {
        m_b_mutable_ = is_mutable;
        m_size_      = size;

        InitializeMainBuffer(size, initial_data);
        InitializeSRV(size);

		if (is_mutable)
		{
			InitializeWriteBuffer(size);
		}

        InitializeReadBuffer(size);
    }

    template <typename T>
    void StructuredBuffer<T>::SetData(UINT size, const T* src_ptr)
    {
        if (!m_b_mutable_)
        {
            throw std::logic_error("StructuredBuffer is defined as not mutable");
        }

        if (m_size_ < size)
        {
            Create(size, nullptr, m_b_mutable_);
        }

        D3D11_MAPPED_SUBRESOURCE mapped_resource;
        GetD3Device().GetContext()->Map(m_write_buffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);

        std::memcpy(mapped_resource.pData, src_ptr, sizeof(T) * size);

        GetD3Device().GetContext()->Unmap(m_write_buffer_.Get(), 0);

        GetD3Device().GetContext()->CopyResource(m_buffer_.Get(), m_write_buffer_.Get());

    }

    template <typename T>
    void StructuredBuffer<T>::GetData(UINT size, T* dst_ptr)
    {
        GetD3Device().GetContext()->CopyResource(m_read_buffer_.Get(), m_buffer_.Get());

        D3D11_MAPPED_SUBRESOURCE mapped_resource;
        GetD3Device().GetContext()->Map(m_read_buffer_.Get(), 0, D3D11_MAP_READ, 0, &mapped_resource);
        std::memcpy(dst_ptr, mapped_resource.pData, sizeof(T) * size);
        GetD3Device().GetContext()->Unmap(m_read_buffer_.Get(), 0);
    }

    template <typename T>
    void StructuredBuffer<T>::Bind(const eShaderType shader)
    {
        GetRenderPipeline().BindResource(which_sb<T>::value, shader, m_srv_.GetAddressOf());
    }

    template <typename T>
    void StructuredBuffer<T>::Unbind(const eShaderType shader)
    {
        GetRenderPipeline().UnbindResource(which_sb<T>::value, shader);
    }

    template <typename T>
    void StructuredBuffer<T>::Clear()
    {
        m_buffer_.Reset();
        m_srv_.Reset();
        m_write_buffer_.Reset();
        m_read_buffer_.Reset();
    }
}
