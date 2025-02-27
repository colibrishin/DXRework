#include "Font.h"
#include "Font.generated.h"

Engine::Resources::Font::Font(const Font& other) : Resource(other)
{
}

Engine::Resources::Font& Engine::Resources::Font::operator=(const Font& other)
{
	return *this;
}

void Engine::Resources::Font::Initialize()
{
}

void Engine::Resources::Font::PreUpdate(const float dt)
{
}

void Engine::Resources::Font::Update(const float dt)
{
}

void Engine::Resources::Font::PostUpdate(const float dt)
{
}

void Engine::Resources::Font::FixedUpdate(const float dt)
{
}

void Engine::Resources::Font::OnSerialized()
{
}

void Engine::Resources::Font::OnDeserialized()
{
}

Engine::IFont& Engine::Resources::Font::GetPrimitive() const
{
	return *m_primitive_;
}

void Engine::Resources::Font::Load_INTERNAL()
{
	IGraphicAPI& gi = s_ga.GetInterface();
	m_primitive_ = Unique<decltype(m_primitive_)::element_type>(gi.GetNewPrimitiveFont());
	if (m_primitive_)
	{
		m_primitive_->Generate(this);
	}
}

void Engine::Resources::Font::Unload_INTERNAL()
{
	m_primitive_.reset();
}
