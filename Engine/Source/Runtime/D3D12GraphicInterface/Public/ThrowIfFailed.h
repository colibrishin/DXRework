#pragma once
#include <exception>
#include <Windows.h>
#include "CoreType.h"

#if WITH_DEBUG
#define SET_NAME( Variable, String )                                                                                   \
    {                                                                                                                  \
        Variable##->SetName( String );                                                                                 \
        Variable##->SetPrivateData(##WKPDID_D3DDebugObjectName##, std::size( String ) - 1, String );                   \
    }
#define SET_NAME_RUNTIME( Variable, String )                                                                     \
    {                                                                                                                  \
        Variable##->SetName( String.c_str() );                                                                         \
        Variable##->SetPrivateData(##WKPDID_D3DDebugObjectName##, std::size( String ), String.c_str() );               \
    }
#else
#define SET_NAME( Variable, String ) Variable##->SetName( String.c_str() );
#define SET_NAME_RUNTIME( Variable, String ) Variable##->SetName( String.c_str() );
#endif

namespace DX
{
	// Helper class for COM exceptions
	class ENGINE_D3D12GRAPHICINTERFACE_API com_exception : public std::exception
	{
	public:
		com_exception(HRESULT hr)
			: result(hr) {}

		const char* what() const noexcept override;

	private:
		HRESULT result;
	};

	ENGINE_D3D12GRAPHICINTERFACE_API void ThrowIfFailed(HRESULT hr);
} // namespace DX