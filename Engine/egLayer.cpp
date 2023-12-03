#include "pch.hpp"

namespace Engine
{
	void Layer::Initialize()
	{
	}

	void Layer::PreUpdate(const float& dt)
	{
		for (const auto& object : m_objects_ | std::views::values)
		{
			if (!object->GetActive())
			{
				continue;
			}

			object->PreUpdate(dt);
		}
	}

	void Layer::Update(const float& dt)
	{
		for (const auto& object : m_objects_ | std::views::values)
		{
			if (!object->GetActive())
			{
				continue;
			}

			object->Update(dt);
		}
	}

	void Layer::PreRender(const float dt)
	{
		for (const auto& object : m_objects_ | std::views::values)
		{
			if (!object->GetActive())
			{
				continue;
			}

			object->PreRender(dt);
		}
	}

	void Layer::Render(const float dt)
	{
		for (const auto& object : m_objects_ | std::views::values)
		{
			if (!object->GetActive())
			{
				continue;
			}

			object->Render(dt);
		}
	}

	void Layer::FixedUpdate(const float& dt)
	{
		for (const auto& object : m_objects_ | std::views::values)
		{
			if (!object->GetActive())
			{
				continue;
			}

			object->FixedUpdate(dt);
		}
	}
}