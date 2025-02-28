#include "Entity.h"
#include "Entity.generated.h"

#if WITH_EDITOR
#include "IUIAPI.h"
#endif

#include "Serialization.hpp"

void Engine::Abstracts::Entity::SetName(const std::string_view name)
{
	m_name_ = name;
#if WITH_EDITOR
	OnNameChanged();
#endif
}

#if WITH_EDITOR
void Engine::Abstracts::Entity::OnNameChanged()
{
	m_ui_info_.label = m_name_ + "##" + std::to_string(GetID());
}
#endif

void Engine::Abstracts::Entity::SetGarbage(bool garbage)
{
	m_b_garbage_ = garbage;
}

const std::filesystem::path& Engine::Abstracts::Entity::GetMetadataPath() const
{
	return m_meta_path_;
}

Engine::GlobalEntityID Engine::Abstracts::Entity::GetID() const
{
	return reinterpret_cast<GlobalEntityID>(this);  // NOLINT(clang-diagnostic-pointer-to-int-cast)
}

const Engine::EntityName& Engine::Abstracts::Entity::GetName() const
{
	return m_name_;
}

bool Engine::Abstracts::Entity::IsGarbage() const
{
	return m_b_garbage_;
}

bool Engine::Abstracts::Entity::IsInitialized() const
{
	return m_b_initialized_;
}

void Engine::Abstracts::Entity::Initialize()
{
	m_b_initialized_ = true;
}

#if WITH_EDITOR
void Engine::Abstracts::Entity::OnUIUpdate( UIContext *const parent, const float dt )
{
    if ( parent )
    {
        IUIAPI &ui = g_ui_accessor.GetInterface();

        ( *parent |= ui.NewLabelAndText( this, "EntityName", { "Name", m_name_, true } ) ).SetFunction( [&]()
        {
            OnNameChanged();
        } );
        ( *parent |= ui.NewButton( this, "EntitySaveButton", { "Save" } ) ).SetFunction( [&]()
        {
            Serializer::Serialize( m_name_, GetSharedPtr<Entity>() );
        } );
        *parent |= ui.NewLabelAndUInt( this, "EntityGlobalID", { "Entity ID", m_precached_id_, 0.f, 0, 0, false } );
        *parent |= ui.NewLabelAndPath( this, "EntityMetadataPath", { "Metadata Path", m_meta_path_ } );
    }
}
#endif

void Engine::Abstracts::Entity::OnSerialized()
{
}

void Engine::Abstracts::Entity::OnDeserialized()
{
}
