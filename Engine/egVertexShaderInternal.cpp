#include "pch.hpp"
#include "egVertexShaderInternal.hpp"

#include "egRenderPipeline.hpp"

SERIALIZER_ACCESS_IMPL(
	Engine::Graphic::VertexShaderInternal,
	_ARTAG(_BSTSUPER(Shader<ID3D11VertexShader>))
	_ARTAG(m_topology_))

namespace Engine::Graphic
{
	VertexShaderInternal::VertexShaderInternal(const EntityName& name,
	                                  const std::filesystem::path& path) : Shader<ID3D11VertexShader>(name, path)
	{
	}

	void VertexShaderInternal::Render(const float dt)
	{
		GetRenderPipeline().SetTopology(m_topology_);
		Shader<ID3D11VertexShader>::Render(dt);
	}
}
