#include "..\Public\Entity.hpp"

SERIALIZE_IMPL
(
 Engine::Abstracts::Entity,
 _ARTAG(m_name_)
 _ARTAG(m_meta_path_)
 _ARTAG(m_b_garbage_)
 _ARTAG(m_b_initialized_)
)

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

Engine::EntityName Engine::Abstracts::Entity::GetName() const
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
