#include "Shader.h"
#include "ShaderModule.h"
#include "ShaderModule.generated.h"

#include <ranges>


#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.h"

#include <magic_enum.hpp>

MODULE_IMPL(Engine::ShaderModule, Shader)

bool Engine::ShaderModule::InitializeImpl()
{

#if WITH_EDITOR
	Managers::ResourceManager::GetInstance().RegisterLoadResource(Engine::Resources::Shader::StaticTypeName(), [](bool& managing_flag)
		{
			const auto& load_callback = [](const std::string_view name, const std::string_view path)
				{
					Managers::ResourceManager::GetInstance().GetResourceByMetadataPath<Engine::Resources::Shader>(path);
				};

			UIHelpers::OpenLoadDialog<Resources::Shader, Managers::ResourceManager>(managing_flag, {}, load_callback, {});
		});

	Managers::ResourceManager::GetInstance().RegisterNewResource(Resources::Shader::StaticTypeName(), [](bool& managing_flag)
		{
			UIInterface& ui = UIInterfaceAccessor::GetInterface();

			static eShaderDomain domain = SHADER_DOMAIN_OPAQUE;

			static bool                 depth_enabled = {};
			static eShaderDepthMode     depth_mode = {};
			static eShaderDepthFunction   depth_function = {};
			static eShaderRasterizerCull  cull = {};
			static eShaderRasterizerDraw  draw = {};
			static eShaderSamplerAddress  sampler_address_mode = {};
			static eShaderSamplerFunction sampler_function = {};
			static eSamplerFilter         sampler_filter = SAMPLER_FILTER_MIN_MAG_MIP_POINT;
			static std::vector<eFormat>   rtv_formats = GetDefaultRTVFormat();
			static eFormat                dsv_format = TEX_FORMAT_D24_UNORM_S8_UINT;
			static ePrimitiveTopology     pt = PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			static ePrimitiveTopologyType ptt = PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			static eSampler               sampler_slot = SAMPLER_TEXTURE;

			const auto& ui_callback = [&](UIContext* const context)
				{
					*context |= ui.NewComboboxUInt8({ "Shader Domain", reinterpret_cast<uint8_t*>(&domain), Resources::Shader::domain_enums.data(), Resources::Shader::domain_enums.size(), true });

					*context |= ui.NewCheckbox({ "Depth Enable", depth_enabled });
					*context |= ui.NewComboboxUInt8({ "Depth Mode", reinterpret_cast<uint8_t*>(&depth_mode), Resources::Shader::depth_mode_enums.data(), Resources::Shader::depth_mode_enums.size(), true });
					*context |= ui.NewCombobox({ "Depth Function", reinterpret_cast<int*>(&depth_function), Resources::Shader::depth_function_enums.data(), Resources::Shader::depth_function_enums.size(), true });

					*context |= ui.NewComboboxUInt8({ "Cull Mode", reinterpret_cast<uint8_t*>(&cull), Resources::Shader::rasterizer_cull_enums.data(), Resources::Shader::rasterizer_cull_enums.size(), true });
					*context |= ui.NewComboboxUInt8({ "Draw Enable", reinterpret_cast<uint8_t*>(&draw), Resources::Shader::rasterizer_draw_enums.data(), Resources::Shader::rasterizer_draw_enums.size(), true });

					*context |= ui.NewCombobox({ "Filter", reinterpret_cast<int*>(&sampler_filter), Resources::Shader::filter_enums.data(), Resources::Shader::filter_enums.size(), true });
					*context |= ui.NewCombobox({ "Sampler Address Mode", reinterpret_cast<int*>(&sampler_address_mode), Resources::Shader::sampler_addr_enums.data(), Resources::Shader::sampler_addr_enums.size(), true });
					*context |= ui.NewCombobox({ "Sampler Function", reinterpret_cast<int*>(&sampler_function), Resources::Shader::sampler_func_enums.data(), Resources::Shader::sampler_func_enums.size(), true });

					{
						*context += ui.NewListBox({ "RenderTarget Format", -1, 0 });
						static std::vector<std::string> index_string;

						if (index_string.size() != rtv_formats.size())
						{
							index_string.clear();
							index_string.resize(rtv_formats.size());
							std::generate_n(index_string.begin(), rtv_formats.size(), [i = 0]() mutable
								{
									return std::to_string(i++);
								});
						}

						for (size_t i = 0; i < rtv_formats.size(); ++i)
						{
							*context |= ui.NewComboboxUInt8({ index_string[i], reinterpret_cast<uint8_t*>(&rtv_formats[i]), Resources::Shader::format_enums.data(), Resources::Shader::format_enums.size(), true });
						}
						-- * context;

						(*context |= ui.NewButton({ context, "Add Render Target" })).SetFunction([]()
							{
								rtv_formats.push_back(GetDefaultRTVFormat().front());
							});
					}

					*context |= ui.NewComboboxUInt8({ "Depth/Stencil Format", reinterpret_cast<uint8_t*>(&dsv_format), Resources::Shader::format_enums.data(), Resources::Shader::format_enums.size(), true });
					*context |= ui.NewCombobox({ "Primitive Topology", reinterpret_cast<int*>(&pt), Resources::Shader::primitive_topology_enum.data(), Resources::Shader::primitive_topology_enum.size(), true });
					*context |= ui.NewCombobox({ "Primitive Topology Type", reinterpret_cast<int*>(&ptt), Resources::Shader::primitive_topology_type_enum.data(), Resources::Shader::primitive_topology_type_enum.size(), true });
					*context |= ui.NewComboboxUInt8({ "Sampler Slot", reinterpret_cast<uint8_t*>(&sampler_slot), Resources::Shader::sampler_slot_enum.data(), Resources::Shader::sampler_slot_enum.size(), true });
				};

			static constexpr auto cleanup_callback = []()
				{
					depth_enabled = {};
					depth_mode = {};
					depth_function = {};
					sampler_filter = SAMPLER_FILTER_MIN_MAG_MIP_POINT;
					sampler_address_mode = {};
					sampler_function = {};
					sampler_filter = {};
					cull = {};
					draw = {};
					rtv_formats = GetDefaultRTVFormat();
					dsv_format = TEX_FORMAT_D24_UNORM_S8_UINT;
					pt = PRIMITIVE_TOPOLOGY_TRIANGLELIST;
					ptt = PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
					sampler_slot = SAMPLER_TEXTURE;
				};

			const auto& load_callback = [](const std::string& name, const std::string& path)
				{
					depth_function = RecastNonlinearEnum<eShaderDepthFunction>(Resources::Shader::depth_function_enums, depth_function);
					sampler_filter = RecastNonlinearEnum<eSamplerFilter>(Resources::Shader::filter_enums, sampler_filter);
					sampler_address_mode = RecastNonlinearEnum<eShaderSamplerAddress>(Resources::Shader::sampler_addr_enums, sampler_address_mode);
					sampler_function = RecastNonlinearEnum<eShaderSamplerFunction>(Resources::Shader::sampler_func_enums, sampler_function);
					sampler_filter = RecastNonlinearEnum<eSamplerFilter>(Resources::Shader::filter_enums, sampler_filter);
					cull = RecastNonlinearEnum<eShaderRasterizerCull>(Resources::Shader::rasterizer_cull_enums, cull);
					draw = RecastNonlinearEnum<eShaderRasterizerDraw>(Resources::Shader::rasterizer_draw_enums, draw);
					for (auto& format : rtv_formats)
					{
						format = RecastNonlinearEnum<eFormat>(Resources::Shader::format_enums, format);
					}
					dsv_format = RecastNonlinearEnum<eFormat>(Resources::Shader::format_enums, dsv_format);
					pt = RecastNonlinearEnum<ePrimitiveTopology>(Resources::Shader::primitive_topology_enum, pt);
					ptt = RecastNonlinearEnum<ePrimitiveTopologyType>(Resources::Shader::primitive_topology_type_enum, ptt);

					if (path.empty())
					{
						return;
					}

					try
					{
						Resources::Shader::Create(
							name, 
							path, 
							domain, 
							depth_enabled,
							depth_mode, 
							depth_function,
							sampler_address_mode,
							sampler_function,
							sampler_filter,
							cull,
							draw,
							rtv_formats, 
							dsv_format,
							pt, 
							ptt, 
							sampler_slot);
					}
					catch (std::exception e)
					{
						cleanup_callback();
					}

					cleanup_callback();
				};

			UIHelpers::OpenNewDialog<Resources::Shader, Managers::ResourceManager>(
				managing_flag,
				ui_callback,
				load_callback,
				cleanup_callback);
		});
#endif

	StockShaderPrecompile();

	return true;
}

void Engine::ShaderModule::StockShaderPrecompile()
{
	Resources::Shader::Create
	(
		"default", "./default.hlsl", SHADER_DOMAIN_OPAQUE,
		true, SHADER_DEPTH_TEST_ALL, SHADER_DEPTH_LESS_EQUAL,
		SHADER_SAMPLER_WRAP, SHADER_SAMPLER_ALWAYS,
		SAMPLER_FILTER_MIN_MAG_MIP_LINEAR,
		SHADER_RASTERIZER_CULL_BACK, SHADER_RASTERIZER_FILL_SOLID,
		GetDefaultRTVFormat(), TEX_FORMAT_D24_UNORM_S8_UINT,
		PRIMITIVE_TOPOLOGY_TRIANGLELIST, PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
	);

	Resources::Shader::Create
	(
		"color", "./color.hlsl", SHADER_DOMAIN_OPAQUE,
		true, SHADER_DEPTH_TEST_ALL, SHADER_DEPTH_LESS_EQUAL,
		SHADER_SAMPLER_WRAP, SHADER_SAMPLER_ALWAYS,
		SAMPLER_FILTER_MIN_MAG_MIP_LINEAR,
		SHADER_RASTERIZER_CULL_BACK, SHADER_RASTERIZER_FILL_SOLID,
		GetDefaultRTVFormat(), TEX_FORMAT_D24_UNORM_S8_UINT,
		PRIMITIVE_TOPOLOGY_TRIANGLELIST, PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
	);

	Resources::Shader::Create
	(
		"skybox", "./skybox.hlsl", SHADER_DOMAIN_OPAQUE,
		true, SHADER_DEPTH_TEST_ALL, SHADER_DEPTH_LESS_EQUAL,
		SHADER_SAMPLER_WRAP, SHADER_SAMPLER_ALWAYS,
		SAMPLER_FILTER_MIN_MAG_MIP_LINEAR,
		SHADER_RASTERIZER_CULL_NONE, SHADER_RASTERIZER_FILL_SOLID,
		GetDefaultRTVFormat(), TEX_FORMAT_D24_UNORM_S8_UINT,
		PRIMITIVE_TOPOLOGY_TRIANGLELIST, PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
	);

	Resources::Shader::Create
	(
		"specular_normal", "./specular_normal.hlsl",
		SHADER_DOMAIN_OPAQUE,
		true, SHADER_DEPTH_TEST_ALL, SHADER_DEPTH_LESS_EQUAL,
		SHADER_SAMPLER_WRAP, SHADER_SAMPLER_ALWAYS,
		SAMPLER_FILTER_MIN_MAG_MIP_LINEAR,
		SHADER_RASTERIZER_CULL_BACK, SHADER_RASTERIZER_FILL_SOLID,
		GetDefaultRTVFormat(), TEX_FORMAT_D24_UNORM_S8_UINT,
		PRIMITIVE_TOPOLOGY_TRIANGLELIST, PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
	);

	Resources::Shader::Create
	(
		"normal", "./normal.hlsl", SHADER_DOMAIN_OPAQUE,
		true, SHADER_DEPTH_TEST_ALL, SHADER_DEPTH_LESS_EQUAL,
		SHADER_SAMPLER_WRAP, SHADER_SAMPLER_ALWAYS,
		SAMPLER_FILTER_MIN_MAG_MIP_LINEAR,
		SHADER_RASTERIZER_CULL_BACK, SHADER_RASTERIZER_FILL_SOLID,
		GetDefaultRTVFormat(), TEX_FORMAT_D24_UNORM_S8_UINT,
		PRIMITIVE_TOPOLOGY_TRIANGLELIST, PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
	);

	Resources::Shader::Create
	(
		"refraction", "./refraction.hlsl", SHADER_DOMAIN_POST_PROCESS,
		true, SHADER_DEPTH_TEST_ALL, SHADER_DEPTH_LESS_EQUAL,
		SHADER_SAMPLER_WRAP, SHADER_SAMPLER_ALWAYS,
		SAMPLER_FILTER_MIN_MAG_MIP_LINEAR,
		SHADER_RASTERIZER_CULL_BACK, SHADER_RASTERIZER_FILL_SOLID,
		GetDefaultRTVFormat(), TEX_FORMAT_D24_UNORM_S8_UINT,
		PRIMITIVE_TOPOLOGY_TRIANGLELIST, PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
	);

	Resources::Shader::Create
	(
		"specular_tex", "./specular_tex.hlsl", SHADER_DOMAIN_OPAQUE,
		true, SHADER_DEPTH_TEST_ALL, SHADER_DEPTH_LESS_EQUAL,
		SHADER_SAMPLER_WRAP, SHADER_SAMPLER_ALWAYS,
		SAMPLER_FILTER_MIN_MAG_MIP_LINEAR,
		SHADER_RASTERIZER_CULL_BACK, SHADER_RASTERIZER_FILL_SOLID,
		GetDefaultRTVFormat(), TEX_FORMAT_D24_UNORM_S8_UINT,
		PRIMITIVE_TOPOLOGY_TRIANGLELIST, PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
	);

	Resources::Shader::Create
	(
		"specular", "./specular.hlsl", SHADER_DOMAIN_OPAQUE,
		true, SHADER_DEPTH_TEST_ALL, SHADER_DEPTH_LESS_EQUAL,
		SHADER_SAMPLER_WRAP, SHADER_SAMPLER_ALWAYS,
		SAMPLER_FILTER_MIN_MAG_MIP_LINEAR,
		SHADER_RASTERIZER_CULL_BACK, SHADER_RASTERIZER_FILL_SOLID,
		GetDefaultRTVFormat(), TEX_FORMAT_D24_UNORM_S8_UINT,
		PRIMITIVE_TOPOLOGY_TRIANGLELIST, PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
	);

	Resources::Shader::Create
	(
		"billboard", "./billboard.hlsl", SHADER_DOMAIN_OPAQUE,
		true, SHADER_DEPTH_TEST_ALL, SHADER_DEPTH_LESS_EQUAL,
		SHADER_SAMPLER_WRAP, SHADER_SAMPLER_ALWAYS,
		SAMPLER_FILTER_MIN_MAG_MIP_LINEAR,
		SHADER_RASTERIZER_CULL_NONE, SHADER_RASTERIZER_FILL_SOLID,
		GetDefaultRTVFormat(), TEX_FORMAT_D24_UNORM_S8_UINT,
		PRIMITIVE_TOPOLOGY_POINTLIST, PRIMITIVE_TOPOLOGY_TYPE_POINT
	);

	Resources::Shader::Create
	(
		"atlas_billboard", "./atlas_billboard.hlsl", SHADER_DOMAIN_OPAQUE,
		true, SHADER_DEPTH_TEST_ALL, SHADER_DEPTH_LESS_EQUAL,
		SHADER_SAMPLER_WRAP, SHADER_SAMPLER_ALWAYS,
		SAMPLER_FILTER_MIN_MAG_MIP_LINEAR,
		SHADER_RASTERIZER_CULL_NONE, SHADER_RASTERIZER_FILL_SOLID,
		GetDefaultRTVFormat(), TEX_FORMAT_D24_UNORM_S8_UINT,
		PRIMITIVE_TOPOLOGY_POINTLIST, PRIMITIVE_TOPOLOGY_TYPE_POINT
	);

	Resources::Shader::Create
	(
		"atlas", "./atlas.hlsl", SHADER_DOMAIN_OPAQUE,
		true, SHADER_DEPTH_TEST_ALL, SHADER_DEPTH_LESS_EQUAL,
		SHADER_SAMPLER_WRAP, SHADER_SAMPLER_ALWAYS,
		SAMPLER_FILTER_MIN_MAG_MIP_LINEAR,
		SHADER_RASTERIZER_CULL_BACK, SHADER_RASTERIZER_FILL_SOLID,
		GetDefaultRTVFormat(), TEX_FORMAT_D24_UNORM_S8_UINT,
		PRIMITIVE_TOPOLOGY_TRIANGLELIST, PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
	);
}

const std::vector<std::string>& Engine::ShaderModule::LoadAfter() const
{
	static std::vector<std::string> load_after{ "RenderPipeline" };
	return load_after;
}

bool Engine::ShaderModule::ShutdownImpl()
{
#if WITH_EDITOR
	Managers::ResourceManager::GetInstance().UnregisterNewResource(Resources::Shader::StaticTypeName());
	Managers::ResourceManager::GetInstance().UnregisterLoadResource(Resources::Shader::StaticTypeName());
#endif
	return true;
}

bool Engine::ShaderModule::DynamicLoadable()
{
	return true;
}
