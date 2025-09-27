#include "ComputeShaderModule.h"
#include "ComputeShaderModule.generated.h"

#include "ResourceManager.h"
#include "ComputeShader.h"

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
			});
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

    const std::vector<std::string>& ComputeShaderModule::LoadAfter() const
    {
        static std::vector<std::string> load_after = { "RenderPipeline" };
        return load_after;
    }
} // namespace Engine::Resources
