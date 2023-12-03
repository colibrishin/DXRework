#pragma once
#include <memory>
#include <Windows.h>
#include <functional>
#include <boost/smart_ptr/weak_ptr.hpp>

// Need to be included before boost only in the header, requires a default constructor
#define SERIALIZER_ACCESS \
	friend class Engine::Serializer; \
	friend class boost::serialization::access; \
	template<class Archive> \
	void serialize(Archive & ar, const unsigned int file_version); \

// part of serialization access implementation
#define SERIALIZER_ACCESS_IMPL(NAMESPACE_TYPE) \
		template void NAMESPACE_TYPE::serialize<boost::archive::text_iarchive>( \
			boost::archive::text_iarchive& ar, \
			const unsigned int file_version); \
		template void NAMESPACE_TYPE::serialize<boost::archive::text_oarchive>( \
			boost::archive::text_oarchive& ar, \
			const unsigned int file_version); \
		BOOST_CLASS_EXPORT_IMPLEMENT(NAMESPACE_TYPE) \


#define _ARTAG(TYPENAME) ar & TYPENAME;
#define _BASEOBJECT(BASE) boost::serialization::base_object<BASE>(*this)

#define SERIALIZER_ACCESS_IMPL2(NAMESPACE_TYPE, ...) \
		template <class Archive> \
		void NAMESPACE_TYPE::serialize(Archive& ar, const unsigned int file_version) \
		{ \
			__VA_ARGS__ \
		} \

// full serialization access implementation for a class only in the cpp file, requires a boost include
#define SERIALIZER_ACCESS_IMPL3(NAMESPACE_TYPE, ...) \
		SERIALIZER_ACCESS_IMPL(NAMESPACE_TYPE) \
		SERIALIZER_ACCESS_IMPL2(NAMESPACE_TYPE, __VA_ARGS__) \

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
	using StrongFont = boost::shared_ptr<Resources::Font>;

	using EntityID = LONG_PTR;

	using TaskSchedulerFunc = std::function<void(const float&)>;
}
