#pragma once
#include <exception>
#include <Windows.h>

namespace DX
{
	// Helper class for COM exceptions
	class com_exception : public std::exception
	{
	public:
		com_exception(HRESULT hr)
			: result(hr) {}

		const char* what() const noexcept override;

	private:
		HRESULT result;
	};

	void ThrowIfFailed(HRESULT hr);
} // namespace DX