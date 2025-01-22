#include "../Public/Texture2D.h"

#include "ModuleManager/Public/ModuleManager.h"

#include "Source/Runtime/Core/ResourceManager/Public/ResourceManager.hpp"
#include <magic_enum.hpp>

bool Engine::Resources::Texture2D::m_b_ui_load_dialog_ = false;

namespace Engine::Resources
{
	Strong<Texture2D> Texture2D::Create(
		const std::string& name, const std::filesystem::path& path, const GenericTextureDescription& desc
	)
	{
		if (const auto pcheck = Managers::ResourceManager::GetInstance().GetResourceByRawPath<Texture2D>(path).lock();
			const auto ncheck = Managers::ResourceManager::GetInstance().GetResource<Texture2D>(name).lock())
		{
			return ncheck;
		}
		const auto obj = boost::make_shared<Texture2D>(path, desc);
		Managers::ResourceManager::GetInstance().AddResource(name, obj);
		return obj;
	}

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

MODULE_IMPL(Engine::Texture2DModule, Texture2D)

void Engine::Texture2DModule::Initialize()
{
	Managers::ResourceManager::GetInstance().RegisterLoadResource("Texture2D", [](bool& managing_flag)
	{
		UIInterface& ui = UIInterfaceAccessor::GetInterface();
		static GenericTextureDescription desc{};
		static constexpr auto tex_format_cstr = []()
		{
			constexpr auto tex_format = magic_enum::enum_names<eFormat>();
			std::array<const char*, tex_format.size()> ret{};
			for (size_t i = 0; i < tex_format.size(); ++i)
			{
				ret[i] = tex_format[i].data();
			}
			return ret;
		}();

		static constexpr auto res_flag_cstr = []()
		{
			constexpr auto res_flag = magic_enum::enum_names<eResourceFlag>();
			std::array<const char*, res_flag.size()> ret{};
			for (size_t i = 0; i < res_flag.size(); ++i)
			{
				ret[i] = res_flag[i].data();
			}
			return ret;
		}();
		static std::array<bool, res_flag_cstr.size()> res_flag_bool{};

		static constexpr auto tex_layout_cstr = []()
		{
			constexpr auto tex_layout = magic_enum::enum_names<eTextureLayout>();
			std::array<const char*, tex_layout.size()> ret{};
			for (size_t i = 0; i < tex_layout.size(); ++i)
			{
				ret[i] = tex_layout[i].data();
			}
			return ret;
		}();

		const auto& ui_callback = [&](UIContext* const context)
		{
			*context |= ui.NewLabelAndULLD({"Alignment", desc.Alignment, 0.1f, 0.f, 0.f, true});
			*context |= ui.NewLabelAndULLD({"Width", desc.Width, 0.1f, 0, 0, true});
			*context |= ui.NewLabelAndUInt({"Height", desc.Height, 0.1f, 0, 0, true});
			*context |= ui.NewLabelAndUInt16({"Depth or Array Size", desc.DepthOrArraySize, 0.1f, 0, 0, true});
			*context |= ui.NewComboboxUInt8({"Format", reinterpret_cast<uint8_t*>(&desc.Format), tex_format_cstr.data(), tex_format_cstr.size()}); // should recast before use, non-linear enum

			{
				*context += ui.NewListBox({"Resource Flags", -1, 0});
				for (size_t i = 0; i < res_flag_cstr.size(); ++i)
				{
					*context |= ui.NewSelectable({res_flag_cstr[i], res_flag_bool[i]});
				}
				--*context;
			}

			*context |= ui.NewLabelAndUInt16({"Mips Level", desc.MipsLevel, 0.1f, 0, 0, true});
			*context |= ui.NewComboboxUInt8({"Texture layout", reinterpret_cast<uint8_t*>(&desc.Layout), tex_layout_cstr.data(), tex_layout_cstr.size()}); // should recast before use, non-linear enum
			*context |= ui.NewLabelAndUInt({"Sampler Count", desc.SampleDesc.Count, 0.1f, 0, 0, true});
			*context |= ui.NewLabelAndUInt({"Sampler Quality", desc.SampleDesc.Quality, 0.1f, 0, 0, true});

			// todo: srv
			// todo: rtv
			// todo: dsv
			// todo: uav
		};

		const auto& load_callback = [&](const std::string& name, const std::string& path)
		{
			try 
			{
				if (!path.empty())
				{
					if (std::filesystem::exists(path))
					{
						Resources::Texture2D::Create(name, path, {});
					}
				}
				else
				{
					if (const auto format_validity = magic_enum::enum_cast<eFormat>(tex_format_cstr[desc.Format]);
						format_validity.has_value())
					{
						desc.Format = format_validity.value();
					}

					if (const auto format_validity = magic_enum::enum_cast<eTextureLayout>(tex_layout_cstr[desc.Layout]); 
						format_validity.has_value())
					{
						desc.Layout = format_validity.value();
					}

					for (size_t i = 0; i < res_flag_cstr.size(); ++i)
					{
						const auto target_flag = magic_enum::enum_cast<eResourceFlag>(res_flag_cstr[i]);

						if (res_flag_bool[i] && target_flag.has_value())
						{
							desc.Flags |= target_flag.value();
						}
					}

					Resources::Texture2D::Create(name, "", desc);
					desc = {};
					res_flag_bool = {};
				}
			}
			catch (std::exception e)
			{
				desc = {};
				return;
			}
		};

		Managers::ResourceManager::GetInstance().OpenNewSimpleDialog<Resources::Texture2D>(managing_flag, ui_callback, load_callback);
	});
}

void Engine::Texture2DModule::Shutdown()
{
	Managers::ResourceManager::GetInstance().UnregisterLoadResource("Texture2D");
}

bool Engine::Texture2DModule::DynamicLoadable()
{
	return false;
}
