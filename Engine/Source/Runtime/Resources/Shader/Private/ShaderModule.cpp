#include "Shader.h"
#include "ShaderModule.h"
#include "ShaderModule.generated.h"

#include <ranges>

#include "ModuleManager/Public/ModuleManager.h"
#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.h"

#include <magic_enum.hpp>

MODULE_IMPL(Engine::ShaderModule, Shader)

template <typename Enum>
constexpr auto CStrEnumStrings()
{
	constexpr auto enum_val = magic_enum::enum_names<Enum>();
	std::array<const char*, enum_val.size()> ret{};
	for (size_t i = 0; i < enum_val.size(); ++i)
	{
		ret[i] = enum_val[i].data();
	}
	return ret;
}

template <typename Enum>
Enum RecastNonlinearEnum(const auto& cstr_array, size_t value)
{
	if (const auto format_validity = magic_enum::enum_cast<Enum>(cstr_array[value]);
		format_validity.has_value())
	{
		return format_validity.value();
	}

	return static_cast<Enum>(0);
}

void Engine::ShaderModule::Initialize()
{
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

			static constexpr auto domain_enums = CStrEnumStrings<eShaderDomain>();
			static constexpr auto depth_enable_enums = CStrEnumStrings<eShaderDepthMode>();
			static constexpr auto depth_function_enums = CStrEnumStrings<eShaderDepthFunction>();
			static constexpr auto rasterizer_cull_enums = CStrEnumStrings<eShaderRasterizerCull>();
			static constexpr auto rasterizer_draw_enums = CStrEnumStrings<eShaderRasterizerDraw>();
			static constexpr auto filter_enums = CStrEnumStrings<eSamplerFilter>();
			static constexpr auto sampler_addr_enums = CStrEnumStrings<eShaderSamplerAddress>();
			static constexpr auto sampler_func_enums = CStrEnumStrings<eShaderSamplerFunction>();
			static constexpr auto format_enums = CStrEnumStrings<eFormat>();
			static constexpr auto primitive_topology_enum = CStrEnumStrings<ePrimitiveTopology>();
			static constexpr auto primitive_topology_type_enum = CStrEnumStrings<ePrimitiveTopologyType>();
			static constexpr auto sampler_slot_enum = CStrEnumStrings<eSampler>();

			static eShaderDomain domain = SHADER_DOMAIN_OPAQUE;

			static bool                 depth_enabled = {};
			static eShaderDepthMode     depth_mode = {};
			static eShaderDepthFunction   depth_function = {};
			static eShaderRasterizerCull  cull = {};
			static eShaderRasterizerDraw  draw = {};
			static eShaderSamplerAddress  sampler_address_mode = {};
			static eShaderSamplerFunction sampler_function = {};
			static eSamplerFilter         sampler_filter = SAMPLER_FILTER_MIN_MAG_MIP_POINT;
			static eShaderRasterizerCull  cull_mode = {};
			static eShaderRasterizerDraw  draw_mode = {};
			static std::vector<eFormat>   rtv_formats = GetDefaultRTVFormat();
			static eFormat                dsv_format = TEX_FORMAT_D24_UNORM_S8_UINT;
			static ePrimitiveTopology     pt = PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			static ePrimitiveTopologyType ptt = PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			static eSampler               sampler_slot = SAMPLER_TEXTURE;

			const auto& ui_callback = [&](UIContext* const context)
				{
					*context |= ui.NewComboboxUInt8({ "Shader Domain", reinterpret_cast<uint8_t*>(&domain), domain_enums.data(), domain_enums.size() });

					*context |= ui.NewCheckbox({ "Depth Enable", depth_enabled });
					*context |= ui.NewComboboxUInt8({ "Depth Mode", reinterpret_cast<uint8_t*>(&depth_mode), depth_enable_enums.data(), depth_enable_enums.size() });
					*context |= ui.NewCombobox({ "Depth Function", reinterpret_cast<int*>(&depth_function), depth_function_enums.data(), depth_function_enums.size() });

					*context |= ui.NewComboboxUInt8({ "Cull Mode", reinterpret_cast<uint8_t*>(&cull), rasterizer_cull_enums.data(), rasterizer_cull_enums.size() });
					*context |= ui.NewComboboxUInt8({ "Draw Enable", reinterpret_cast<uint8_t*>(&draw), rasterizer_draw_enums.data(), rasterizer_draw_enums.size() });

					*context |= ui.NewCombobox({ "Filter", reinterpret_cast<int*>(&sampler_filter), filter_enums.data(), filter_enums.size() });
					*context |= ui.NewCombobox({ "Sampler Address Mode", reinterpret_cast<int*>(&sampler_address_mode), sampler_addr_enums.data(), sampler_addr_enums.size() });
					*context |= ui.NewCombobox({ "Sampler Function", reinterpret_cast<int*>(&sampler_function), sampler_func_enums.data(), sampler_func_enums.size() });

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
							*context |= ui.NewComboboxUInt8({ index_string[i], reinterpret_cast<uint8_t*>(&rtv_formats[i]), format_enums.data(), format_enums.size() });
						}
						-- * context;

						(*context |= ui.NewButton({ "Add Render Target" })).SetFunction([]()
							{
								rtv_formats.push_back(GetDefaultRTVFormat().front());
							});
					}

					*context |= ui.NewComboboxUInt8({ "Depth/Stencil Format", reinterpret_cast<uint8_t*>(&dsv_format), format_enums.data(), format_enums.size() });
					*context |= ui.NewCombobox({ "Primitive Topology", reinterpret_cast<int*>(&pt), primitive_topology_enum.data(), primitive_topology_enum.size() });
					*context |= ui.NewCombobox({ "Primitive Topology Type", reinterpret_cast<int*>(&ptt), primitive_topology_type_enum.data(), primitive_topology_type_enum.size() });
					*context |= ui.NewComboboxUInt8({ "Sampler Slot", reinterpret_cast<uint8_t*>(&sampler_slot), sampler_slot_enum.data(), sampler_slot_enum.size() });
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
					depth_function = RecastNonlinearEnum<eShaderDepthFunction>(depth_function_enums, depth_function);
					sampler_filter = RecastNonlinearEnum<eSamplerFilter>(filter_enums, sampler_filter);
					sampler_address_mode = RecastNonlinearEnum<eShaderSamplerAddress>(sampler_addr_enums, sampler_address_mode);
					sampler_function = RecastNonlinearEnum<eShaderSamplerFunction>(sampler_func_enums, sampler_function);
					sampler_filter = RecastNonlinearEnum<eSamplerFilter>(filter_enums, sampler_filter);
					cull = RecastNonlinearEnum<eShaderRasterizerCull>(rasterizer_cull_enums, cull);
					draw = RecastNonlinearEnum<eShaderRasterizerDraw>(rasterizer_draw_enums, draw);
					for (auto& format : rtv_formats)
					{
						format = RecastNonlinearEnum<eFormat>(format_enums, format);
					}
					dsv_format = RecastNonlinearEnum<eFormat>(format_enums, dsv_format);
					pt = RecastNonlinearEnum<ePrimitiveTopology>(primitive_topology_enum, pt);
					ptt = RecastNonlinearEnum<ePrimitiveTopologyType>(primitive_topology_type_enum, ptt);

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

		StockShaderPrecompile();
}

void Engine::ShaderModule::StockShaderPrecompile()
{
	Resources::Shader::Create<true>
	(
		"default", "./default.hlsl", SHADER_DOMAIN_OPAQUE,
		true, SHADER_DEPTH_TEST_ALL, SHADER_DEPTH_LESS_EQUAL,
		SHADER_SAMPLER_WRAP, SHADER_SAMPLER_ALWAYS,
		SAMPLER_FILTER_MIN_MAG_MIP_LINEAR,
		SHADER_RASTERIZER_CULL_BACK, SHADER_RASTERIZER_FILL_SOLID,
		GetDefaultRTVFormat(), TEX_FORMAT_D24_UNORM_S8_UINT,
		PRIMITIVE_TOPOLOGY_TRIANGLELIST, PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
	);

	Resources::Shader::Create<true>
	(
		"color", "./color.hlsl", SHADER_DOMAIN_OPAQUE,
		true, SHADER_DEPTH_TEST_ALL, SHADER_DEPTH_LESS_EQUAL,
		SHADER_SAMPLER_WRAP, SHADER_SAMPLER_ALWAYS,
		SAMPLER_FILTER_MIN_MAG_MIP_LINEAR,
		SHADER_RASTERIZER_CULL_BACK, SHADER_RASTERIZER_FILL_SOLID,
		GetDefaultRTVFormat(), TEX_FORMAT_D24_UNORM_S8_UINT,
		PRIMITIVE_TOPOLOGY_TRIANGLELIST, PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
	);

	Resources::Shader::Create<true>
	(
		"skybox", "./skybox.hlsl", SHADER_DOMAIN_OPAQUE,
		true, SHADER_DEPTH_TEST_ALL, SHADER_DEPTH_LESS_EQUAL,
		SHADER_SAMPLER_WRAP, SHADER_SAMPLER_ALWAYS,
		SAMPLER_FILTER_MIN_MAG_MIP_LINEAR,
		SHADER_RASTERIZER_CULL_NONE, SHADER_RASTERIZER_FILL_SOLID,
		GetDefaultRTVFormat(), TEX_FORMAT_D24_UNORM_S8_UINT,
		PRIMITIVE_TOPOLOGY_TRIANGLELIST, PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
	);

	Resources::Shader::Create<true>
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

	Resources::Shader::Create<true>
	(
		"normal", "./normal.hlsl", SHADER_DOMAIN_OPAQUE,
		true, SHADER_DEPTH_TEST_ALL, SHADER_DEPTH_LESS_EQUAL,
		SHADER_SAMPLER_WRAP, SHADER_SAMPLER_ALWAYS,
		SAMPLER_FILTER_MIN_MAG_MIP_LINEAR,
		SHADER_RASTERIZER_CULL_BACK, SHADER_RASTERIZER_FILL_SOLID,
		GetDefaultRTVFormat(), TEX_FORMAT_D24_UNORM_S8_UINT,
		PRIMITIVE_TOPOLOGY_TRIANGLELIST, PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
	);

	Resources::Shader::Create<true>
	(
		"refraction", "./refraction.hlsl", SHADER_DOMAIN_POST_PROCESS,
		true, SHADER_DEPTH_TEST_ALL, SHADER_DEPTH_LESS_EQUAL,
		SHADER_SAMPLER_WRAP, SHADER_SAMPLER_ALWAYS,
		SAMPLER_FILTER_MIN_MAG_MIP_LINEAR,
		SHADER_RASTERIZER_CULL_BACK, SHADER_RASTERIZER_FILL_SOLID,
		GetDefaultRTVFormat(), TEX_FORMAT_D24_UNORM_S8_UINT,
		PRIMITIVE_TOPOLOGY_TRIANGLELIST, PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
	);

	Resources::Shader::Create<true>
	(
		"specular_tex", "./specular_tex.hlsl", SHADER_DOMAIN_OPAQUE,
		true, SHADER_DEPTH_TEST_ALL, SHADER_DEPTH_LESS_EQUAL,
		SHADER_SAMPLER_WRAP, SHADER_SAMPLER_ALWAYS,
		SAMPLER_FILTER_MIN_MAG_MIP_LINEAR,
		SHADER_RASTERIZER_CULL_BACK, SHADER_RASTERIZER_FILL_SOLID,
		GetDefaultRTVFormat(), TEX_FORMAT_D24_UNORM_S8_UINT,
		PRIMITIVE_TOPOLOGY_TRIANGLELIST, PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
	);

	Resources::Shader::Create<true>
	(
		"specular", "./specular.hlsl", SHADER_DOMAIN_OPAQUE,
		true, SHADER_DEPTH_TEST_ALL, SHADER_DEPTH_LESS_EQUAL,
		SHADER_SAMPLER_WRAP, SHADER_SAMPLER_ALWAYS,
		SAMPLER_FILTER_MIN_MAG_MIP_LINEAR,
		SHADER_RASTERIZER_CULL_BACK, SHADER_RASTERIZER_FILL_SOLID,
		GetDefaultRTVFormat(), TEX_FORMAT_D24_UNORM_S8_UINT,
		PRIMITIVE_TOPOLOGY_TRIANGLELIST, PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
	);

	Resources::Shader::Create<true>
	(
		"billboard", "./billboard.hlsl", SHADER_DOMAIN_OPAQUE,
		true, SHADER_DEPTH_TEST_ALL, SHADER_DEPTH_LESS_EQUAL,
		SHADER_SAMPLER_WRAP, SHADER_SAMPLER_ALWAYS,
		SAMPLER_FILTER_MIN_MAG_MIP_LINEAR,
		SHADER_RASTERIZER_CULL_NONE, SHADER_RASTERIZER_FILL_SOLID,
		GetDefaultRTVFormat(), TEX_FORMAT_D24_UNORM_S8_UINT,
		PRIMITIVE_TOPOLOGY_POINTLIST, PRIMITIVE_TOPOLOGY_TYPE_POINT
	);

	/*
	constexpr TEX_FORMAT intensity_rtv_formats[]
	{
		TEX_FORMAT_R32G32B32A32_UINT,
		TEX_FORMAT_R32G32B32A32_FLOAT
	};

	Resources::Shader::Create
	(
		"intensity_test", "./intensity_test.hlsl", SHADER_DOMAIN_OPAQUE,
		SHADER_DEPTH_TEST_ALL | SHADER_DEPTH_LESS_EQUAL,
		SHADER_RASTERIZER_CULL_BACK | SHADER_RASTERIZER_FILL_SOLID,
		SAMPLER_FILTER_MIN_MAG_MIP_POINT,
		SHADER_SAMPLER_WRAP | SHADER_SAMPLER_NEVER,
		intensity_rtv_formats, 2, TEX_FORMAT_D32_FLOAT,
		PRIMITIVE_TOPOLOGY_TRIANGLELIST, PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
	);
	*/

	Resources::Shader::Create<true>
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

void Engine::ShaderModule::Shutdown()
{
	Managers::ResourceManager::GetInstance().UnregisterNewResource(Resources::Shader::StaticTypeName());
	Managers::ResourceManager::GetInstance().UnregisterLoadResource(Resources::Shader::StaticTypeName());
}

bool Engine::ShaderModule::DynamicLoadable()
{
	return true;
}
