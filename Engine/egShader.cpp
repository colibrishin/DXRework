#include "pch.hpp"
#include "egShader.hpp"

#include "egD3Device.hpp"

namespace Engine::Graphic
{
	// explicit template instantiation
	template class Shader<ID3D11VertexShader>;
	template class Shader<ID3D11PixelShader>;
	template class Shader<ID3D11GeometryShader>;
	template class Shader<ID3D11ComputeShader>;
	template class Shader<ID3D11HullShader>;
	template class Shader<ID3D11DomainShader>;

	template <typename T>
	void Shader<T>::Load_INTERNAL()
	{
		D3Device::CreateShader(GetPath(), this);
	}
}
