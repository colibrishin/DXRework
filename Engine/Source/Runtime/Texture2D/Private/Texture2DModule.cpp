#include "Texture2DModule.h"
#include "Texture2DModule.generated.h"

#include "Texture2D.h"

#include "ResourceManager.h"
#include <magic_enum.hpp>

MODULE_IMPL(Engine::Texture2DModule, Texture2D)

bool Engine::Texture2DModule::InitializeImpl()
{
#if WITH_EDITOR
	Managers::ResourceManager::GetInstance().RegisterLoadResource(Engine::Resources::Texture2D::StaticTypeName(), [](bool& managing_flag)
		{
			const auto& load_callback = [](const std::string_view name, const std::string_view path)
				{
					Managers::ResourceManager::GetInstance().GetResourceByMetadataPath<Engine::Resources::Texture2D>(path);
				};

			UIHelpers::OpenLoadDialog<Resources::Texture2D, Managers::ResourceManager>(managing_flag, {}, load_callback, {});
		});

	Managers::ResourceManager::GetInstance().RegisterNewResource(Engine::Resources::Texture2D::StaticTypeName(), [](bool& managing_flag)
	{
		IUIAPI& ui = s_uia.GetInterface();
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

		static constexpr auto cleanup_callback = []()
			{
				desc = {};
			};

		const auto& ui_callback = [&](UIContext* const context)
		{
            *context |= ui.NewLabelAndULLD( nullptr,
                                            "TextureAlignmentCombobox",
                                            { "Alignment", desc.Alignment, 0.1f, 0.f, 0.f, true } );
            *context |= ui.NewLabelAndULLD( nullptr,
                                            "TextureWidthCombobox",
                                            { "Width", desc.Width, 0.1f, 0, 0, true } );
            *context |= ui.NewLabelAndUInt( nullptr,
                                            "TextureHeightCombobox",
                                            { "Height", desc.Height, 0.1f, 0, 0, true } );
            *context |= ui.NewLabelAndUInt16( nullptr,
                                              "TextureSizeCombobox",
                                              { "Depth or Array Size", desc.DepthOrArraySize, 0.1f, 0, 0, true } );
            *context |= ui.NewCombobox( nullptr,
                                        "TextureFormatCombobox",
                                        { "Format",
                                          reinterpret_cast<int *>( &desc.Format ),
                                          tex_format_cstr.data(),
                                          tex_format_cstr.size(),
                                          true } ); // should recast before use, non-linear enum

            {
                *context += ui.NewListBox( nullptr, "TextureResourceFlagListBox", { "Resource Flags", -1, 0 } );
                for ( size_t i = 0; i < res_flag_cstr.size(); ++i )
                {
                    *context |= ui.NewSelectable( nullptr,
                                                  std::format( "TextureResourceFlag", i ),
                                                  { res_flag_cstr[ i ], res_flag_bool[ i ] } );
                }
                --*context;
            }

            *context |= ui.NewLabelAndUInt16( nullptr,
                                              "TextureMipsLevel",
                                              { "Mips Level", desc.MipsLevel, 0.1f, 0, 0, true } );
            *context |= ui.NewComboboxUInt8( nullptr,
                                             "TextureLayoutCombobox",
                                             { "Texture layout",
                                               reinterpret_cast<uint8_t *>( &desc.Layout ),
                                               tex_layout_cstr.data(),
                                               tex_layout_cstr.size(),
                                               true } ); // should recast before use, non-linear enum
            *context |= ui.NewLabelAndUInt( nullptr,
                                            "TextureSamplerCount",
                                            { "Sampler Count", desc.SampleDesc.Count, 0.1f, 0, 0, true } );
            *context |= ui.NewLabelAndUInt( nullptr,
                                            "TextureSamplerQuality",
                                            { "Sampler Quality", desc.SampleDesc.Quality, 0.1f, 0, 0, true } );

            // todo: srv
            // todo: rtv
            // todo: dsv
            // todo: uav
		};

        const auto &load_callback = [&]( const std::string &name, const std::string &path )
        {
            try
            {
                if ( !path.empty() )
                {
                    if ( std::filesystem::exists( path ) )
                    {
                        Resources::Texture2D::Create( name, path, GenericTextureDescription{} );
                    }
                }
                else
                {
                    if ( const auto format_validity = magic_enum::enum_cast<eFormat>( tex_format_cstr[ desc.Format ] );
                        format_validity.has_value() )
                    {
                        desc.Format = format_validity.value();
                    }

                    if ( const auto format_validity = magic_enum::enum_cast<eTextureLayout>(
                                tex_layout_cstr[ desc.Layout ] );
                        format_validity.has_value() )
                    {
                        desc.Layout = format_validity.value();
                    }

                    for ( size_t i = 0; i < res_flag_cstr.size(); ++i )
                    {
                        const auto target_flag = magic_enum::enum_cast<eResourceFlag>( res_flag_cstr[ i ] );

                        if ( res_flag_bool[ i ] && target_flag.has_value() )
                        {
                            desc.Flags |= target_flag.value();
                        }
                    }

                    Resources::Texture2D::Create( name, "", desc );
                    cleanup_callback();
                    res_flag_bool = {};
                }
            }
            catch ( std::exception e )
            {
                cleanup_callback();
                return;
            }
        };

        UIHelpers::OpenNewDialog<Resources::Texture2D, Managers::ResourceManager>(
                managing_flag,
                ui_callback,
                load_callback,
                cleanup_callback );
    } );
#endif

    return true;
}

bool Engine::Texture2DModule::ShutdownImpl()
{
#if WITH_EDITOR
	Managers::ResourceManager::GetInstance().UnregisterLoadResource(Engine::Resources::Texture2D::StaticTypeName());
	Managers::ResourceManager::GetInstance().UnregisterNewResource(Engine::Resources::Texture2D::StaticTypeName());
#endif
	return true;
}

bool Engine::Texture2DModule::DynamicLoadable()
{
	return false;
}

const std::vector<std::string> &Engine::Texture2DModule::LoadAfter() const
{
    static std::vector<std::string> load_after = { "RenderPipeline", "Texture" };
    return load_after;
}
