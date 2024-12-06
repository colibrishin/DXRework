#include "../Public/ThrowIfFailed.h"
#include <stdio.h>

namespace DX
{
	const char* com_exception::what() const noexcept
	{
		static char s_str[64] = {};
		sprintf_s
				(
				 s_str, "Failure with HRESULT of %08X",
				 static_cast<unsigned int>(result)
				);
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