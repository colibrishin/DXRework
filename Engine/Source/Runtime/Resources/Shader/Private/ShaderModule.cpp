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

			Managers::ResourceManager::GetInstance().OpenLoadDialog<Engine::Resources::Shader>(managing_flag, {}, load_callback, {});
		});

	Managers::ResourceManager::GetInstance().RegisterNewResource(Resources::Shader::StaticTypeName(), [](bool& managing_flag)
		{
			UIInterface& ui = UIInterfaceAccessor::GetInterface();

			static constexpr auto domain_enums = CStrEnumStrings<eShaderDomain>();
			static constexpr auto depth_enable_enums = CStrEnumStrings<eShaderDepthEnable>();
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

			static eShaderDepths          depths_combined = {};
			static eShaderDepthEnable     depth_enabled = {};
			static eShaderDepthFunction   depth_function = {};
			static eShaderRasterizers     rasterizers_combined = {};
			static eShaderRasterizerCull  cull = {};
			static eShaderRasterizerDraw  draw = {};
			static eSamplerFilter         sampler_filter = SAMPLER_FILTER_MIN_MAG_MIP_POINT;
			static eShaderSamplers        sampler_combined = {};
			static UINT                   sampler_addr_mode = {};
			static UINT                   sampler_func = {};
			static std::vector<eFormat>   rtv_formats = GetDefaultRTVFormat();
			static eFormat                dsv_format = TEX_FORMAT_D24_UNORM_S8_UINT;
			static ePrimitiveTopology     pt = PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			static ePrimitiveTopologyType ptt = PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			static eSampler               sampler_slot = SAMPLER_TEXTURE;

			const auto& ui_callback = [&](UIContext* const context)
				{
					*context |= ui.NewComboboxUInt8({ "Shader Domain", reinterpret_cast<uint8_t*>(&domain), domain_enums.data(), domain_enums.size() });

					*context |= ui.NewComboboxUInt8({ "Depth Enable", reinterpret_cast<uint8_t*>(&depth_enabled), depth_enable_enums.data(), depth_enable_enums.size() });
					*context |= ui.NewCombobox({ "Depth Function", reinterpret_cast<int*>(&depth_function), depth_function_enums.data(), depth_function_enums.size() });

					*context |= ui.NewComboboxUInt8({ "Cull Mode", reinterpret_cast<uint8_t*>(&cull), rasterizer_cull_enums.data(), rasterizer_cull_enums.size() });
					*context |= ui.NewComboboxUInt8({ "Draw Enable", reinterpret_cast<uint8_t*>(&draw), rasterizer_draw_enums.data(), rasterizer_draw_enums.size() });

					*context |= ui.NewCombobox({ "Filter", reinterpret_cast<int*>(&sampler_filter), filter_enums.data(), filter_enums.size() });

					*context |= ui.NewCombobox({ "Sampler Address Mode", reinterpret_cast<int*>(&sampler_addr_mode), sampler_addr_enums.data(), sampler_addr_enums.size() });
					*context |= ui.NewCombobox({ "Sampler Function", reinterpret_cast<int*>(&sampler_func), sampler_func_enums.data(), sampler_func_enums.size() });

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
					depths_combined = {};
					depth_enabled = {};
					depth_function = {};
					rasterizers_combined = {};
					cull = {};
					draw = {};
					sampler_filter = SAMPLER_FILTER_MIN_MAG_MIP_POINT;
					sampler_combined = {};
					sampler_addr_mode = {};
					sampler_func = {};
					rtv_formats = GetDefaultRTVFormat();
					dsv_format = TEX_FORMAT_D24_UNORM_S8_UINT;
					pt = PRIMITIVE_TOPOLOGY_TRIANGLELIST;
					ptt = PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
					sampler_slot = SAMPLER_TEXTURE;
				};

			const auto& load_callback = [](const std::string& name, const std::string& path)
				{
					depths_combined = depth_enabled | depth_function;
					rasterizers_combined = cull | draw;
					sampler_combined = sampler_addr_mode | sampler_func;

					pt = RecastNonlinearEnum<ePrimitiveTopology>(primitive_topology_enum, pt);
					ptt = RecastNonlinearEnum<ePrimitiveTopologyType>(primitive_topology_type_enum, ptt);

					if (path.empty())
					{
						return;
					}

					try
					{
						Resources::Shader::Create(name, path, domain, depths_combined, rasterizers_combined, sampler_filter, sampler_combined, rtv_formats, dsv_format, pt, ptt, sampler_slot);
					}
					catch (std::exception e)
					{
						cleanup_callback();
					}

					cleanup_callback();
				};

			Managers::ResourceManager::GetInstance().OpenNewDialog<Resources::Shader>(
				managing_flag,
				ui_callback,
				load_callback,
				cleanup_callback);
		});
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
