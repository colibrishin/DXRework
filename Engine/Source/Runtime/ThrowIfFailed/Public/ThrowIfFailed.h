#pragma once
#include <exception>
#include <Windows.h>
#include "Source/Runtime/Misc.h"

namespace DX
{
	// Helper class for COM exceptions
	class THROWIFFAILED_API com_exception : public std::exception
	{
	public:
		com_exception(HRESULT hr)
			: result(hr) {}

		const char* what() const noexcept override;

	private:
		HRESULT result;
	};

	THROWIFFAILED_API void ThrowIfFailed(HRESULT hr);
} // namespace DX