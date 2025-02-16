#include "ComputeShaderModule.h"
#include "ComputeShaderModule.generated.h"

#include "ResourceManager/Public/ResourceManager.h"
#include "ComputeShader.h"

MODULE_IMPL(Engine::ComputeShaderModule, ComputeShader)

namespace Engine
{
	bool ComputeShaderModule::InitializeImpl()
	{
		Managers::ResourceManager::GetInstance().RegisterLoadResource(Resources::ComputeShader::StaticTypeName(), [](bool& managing_flag)
			{
				const auto& load_callback = [](const std::string_view name, const std::string_view path)
					{ Resources::ComputeShader::GetByMetadataPath( path );
					};

				return UIHelpers::OpenLoadDialog<Resources::ComputeShader, Managers::ResourceManager>(
                            managing_flag, {}, load_callback, {} );
			});

		return true;
	}

	bool ComputeShaderModule::ShutdownImpl()
	{
        Managers::ResourceManager::GetInstance().UnregisterLoadResource( Resources::ComputeShader::StaticTypeName() );

		return true;
	}

	bool ComputeShaderModule::DynamicLoadable()
	{
		return true;
	}
} // namespace Engine::Resources
