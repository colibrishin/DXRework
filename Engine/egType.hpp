#pragma once
#include <memory>
#include <Windows.h>
#include <functional>

namespace Engine
{
	namespace Component
	{
		class Collider;
	}

	class Scene;

	namespace Resources
	{
		class Mesh;
	}

	namespace Abstract
	{
		class Object;
		class Component;
		class Resource;
	}

	using WeakObject = std::weak_ptr<Abstract::Object>;
	using WeakComponent = std::weak_ptr<Abstract::Component>;
	using WeakResource = std::weak_ptr<Abstract::Resource>;
	using WeakMesh = std::weak_ptr<Resources::Mesh>;
	using WeakScene = std::weak_ptr<Scene>;
	using WeakCollider = std::weak_ptr<Component::Collider>;

	using StrongObject = std::shared_ptr<Abstract::Object>;
	using StrongComponent = std::shared_ptr<Abstract::Component>;
	using StrongResource = std::shared_ptr<Abstract::Resource>;

	using EntityID = LONG_PTR;

	using TaskSchedulerFunc = std::function<void(const float&)>;
}
