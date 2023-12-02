#pragma once
#include <memory>
#include <Windows.h>
#include <functional>
#include <boost/smart_ptr/weak_ptr.hpp>

namespace Engine
{
	class Layer;

	namespace Objects
	{
		class Light;
		class Camera;
	}

	namespace Component
	{
		class Collider;
	}

	class Scene;

	namespace Resources
	{
		class Font;
		class Mesh;
	}

	namespace Abstract
	{
		class Object;
		class Component;
		class Resource;
	}

	using WeakObject = boost::weak_ptr<Abstract::Object>;
	using WeakComponent = boost::weak_ptr<Abstract::Component>;
	using WeakResource = boost::weak_ptr<Abstract::Resource>;
	using WeakMesh = boost::weak_ptr<Resources::Mesh>;
	using WeakScene = boost::weak_ptr<Scene>;
	using WeakCollider = boost::weak_ptr<Component::Collider>;
	using WeakFont = boost::weak_ptr<Resources::Font>;
	using WeakCamera = boost::weak_ptr<Objects::Camera>;

	using StrongObject = boost::shared_ptr<Abstract::Object>;
	using StrongComponent = boost::shared_ptr<Abstract::Component>;
	using StrongResource = boost::shared_ptr<Abstract::Resource>;
	using StrongScene = boost::shared_ptr<Scene>;
	using StrongLight = boost::shared_ptr<Objects::Light>;
	using StrongLayer = boost::shared_ptr<Layer>;
	using StrongCamera = boost::shared_ptr<Objects::Camera>;

	using EntityID = LONG_PTR;

	using TaskSchedulerFunc = std::function<void(const float&)>;
}
