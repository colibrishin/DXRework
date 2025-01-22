#include "..\Public\Entity.hpp"

#include "UIInterface.h"

void Engine::Abstracts::Entity::SetName(const EntityName& name)
{
	m_name_ = name;
}

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

Engine::TypeName Engine::Abstracts::Entity::GetTypeName() const
{
	return typeid(*this).name();
}

Engine::TypeName Engine::Abstracts::Entity::GetPrettyTypeName() const
{
	const auto type_name = GetTypeName();
	const auto pos       = type_name.find_last_of(":");

	// case where there is no namespace.
	if (pos == std::string::npos)
	{
		return type_name;
	}

	return type_name.substr(pos + 1);
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

void Engine::Abstracts::Entity::OnUIUpdate(UIContext* const parent, const float dt)
{
#if WITH_EDITOR
	if (parent)
	{
		UIInterface& ui = UIInterfaceAccessor::GetInterface();
		GlobalEntityID id = GetID();

		*parent |= ui.NewLabelAndText({"Name", m_name_, true});
		*parent |= ui.NewLabelAndUInt({"Entity ID", id, false});
		*parent |= ui.NewLabelAndPath({"Metadata Path", m_meta_path_});
		(*parent |= ui.NewButton({"Save"})).SetFunction([]()
		{
			// todo: trigger save
			__nop();
		});
	}
#endif
}

void Engine::Abstracts::Entity::OnSerialized()
{
}

void Engine::Abstracts::Entity::OnDeserialized()
{
}
