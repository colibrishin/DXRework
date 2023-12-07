#include "pch.hpp"
#include "egVertexShaderInternal.hpp"

SERIALIZER_ACCESS_IMPL(
	Engine::Graphic::VertexShaderInternal,
	_ARTAG(_BSTSUPER(Shader<ID3D11VertexShader>)));

namespace Engine::Graphic
{
	VertexShaderInternal::VertexShaderInternal(const EntityName& name,
	                                  const std::filesystem::path& path) : Shader<ID3D11VertexShader>(name, path)
	{
	}
}
