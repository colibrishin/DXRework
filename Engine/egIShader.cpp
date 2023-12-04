#include "pch.hpp"
#include "egIShader.hpp"

#include "egManagerHelper.hpp"

namespace Engine::Graphic
{
	IShader::IShader(const EntityName& name, const std::filesystem::path& path): Resource(path, RESOURCE_PRIORITY_SHADER)
	{
		SetName(name);
		m_path_ = path;
	}

	void IShader::Render(const float dt)
	{
		GetRenderPipeline().SetShader(this);
	}
}
