#pragma once
#include <memory>
#include <Windows.h>

namespace Engine
{
	namespace Abstract
	{
		class Object;
		class Component;
		class Resource;
	}

	using WeakObject = std::weak_ptr<Abstract::Object>;
	using WeakComponent = std::weak_ptr<Abstract::Component>;
	using WeakResource = std::weak_ptr<Abstract::Resource>;

	using StrongObject = std::shared_ptr<Abstract::Object>;
	using StrongComponent = std::shared_ptr<Abstract::Component>;
	using StrongResource = std::shared_ptr<Abstract::Resource>;

	using EntityID = LONG_PTR;
}
