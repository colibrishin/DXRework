#include "ComputeShaderModule.h"

#include "ComputeShader.h"
#include "ModuleRegistration.h"
#include "ResourceManager.h"

MODULE_IMPL(Engine::ComputeShaderModule, ComputeShader)

namespace Engine
{
	bool ComputeShaderModule::InitializeImpl()
	{
#if WITH_EDITOR
		Managers::ResourceManager::GetInstance().RegisterLoadResource(Resources::ComputeShader::StaticTypeName(), [](bool& managing_flag)
			{
				const auto& load_callback = [](const std::string_view name, const std::string_view path)
					{ Resources::ComputeShader::GetByMetadataPath( path );
					};

				return UIHelpers::OpenLoadDialog<Resources::ComputeShader, Managers::ResourceManager>(
                            managing_flag, {}, load_callback, {} );
			} ENGINE_MODULE_SCOPE );
#endif

		return true;
	}

	bool ComputeShaderModule::ShutdownImpl()
	{
#if WITH_EDITOR
        Managers::ResourceManager::GetInstance().UnregisterLoadResource( Resources::ComputeShader::StaticTypeName() );
#endif
		return true;
	}

	bool ComputeShaderModule::DynamicLoadable()
	{
		return true;
    }

} // namespace Engine::Resources
