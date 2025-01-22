#include "ComputeShaderModule.h"
#include "ComputeShaderModule.generated.h"

#include "ResourceManager/Public/ResourceManager.h"
#include "ComputeShader.h"

namespace Engine::Resources
{
	void Engine::Resources::ComputeShaderModule::Initialize()
	{
		Managers::ResourceManager::GetInstance().RegisterLoadResource(ComputeShader::StaticTypeName(), [](bool& managing_flag)
			{
				const auto& load_callback = [](const std::string_view name, const std::string_view path)
					{
						ComputeShader::GetByMetadataPath(path);
					};

				return UIHelpers::OpenLoadDialog<ComputeShader, Managers::ResourceManager>(managing_flag, {}, load_callback, {});
			});
	}

	void ComputeShaderModule::Shutdown()
	{
		Managers::ResourceManager::GetInstance().UnregisterLoadResource(ComputeShader::StaticTypeName());
	}

	bool ComputeShaderModule::DynamicLoadable()
	{
		return false;
	}
} // namespace Engine::Resources
