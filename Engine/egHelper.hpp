#pragma once
#include "egApplication.hpp"

namespace Engine
{
	inline float GetDeltaTime()
	{
		return Application::GetDeltaTime();
	}

	template <typename T, typename... Arg>
	inline static std::shared_ptr<T> Instantiate(Arg&&... args)
	{
		const auto object = std::make_shared<T>(std::forward<Arg>(args)...);
		object->Initialize();
		return object;
	}

	static bool IsSamePolarity(const float v1, const float v2)
	{
		return std::copysign(1.0f, v1) == std::copysign(1.0f, v2);
	}
}
