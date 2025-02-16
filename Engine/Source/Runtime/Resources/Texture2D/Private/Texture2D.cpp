#include "../Public/Texture2D.h"
#include "Texture2D.generated.h"

#include "ModuleManager/Public/ModuleManager.h"

#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.h"

bool Engine::Resources::Texture2D::m_b_ui_load_dialog_ = false;

namespace Engine::Resources
{
	UINT64 Texture2D::GetWidth() const
	{
		return Texture::GetWidth();
	}

	UINT Texture2D::GetHeight() const
	{
		return Texture::GetHeight();
	}

	void Texture2D::Load_INTERNAL()
	{
		const auto& gd = GetDescription();

		if (GetPath().empty() && !(gd.Width + gd.Height))
		{
			throw std::logic_error("Hotloading texture should be define in width, height");
		}

		Texture::Load_INTERNAL();
	}

	void Texture2D::Unload_INTERNAL()
	{
		Texture::Unload_INTERNAL();
	}

	UINT Texture2D::GetDepth() const
	{
		return Texture::GetDepth();
	}
}
