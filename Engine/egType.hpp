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

// part of serialization access implementation, forward declaration of serialize function
#define SERIALIZER_ACCESS_IMPL1(NAMESPACE_TYPE) \
		template void NAMESPACE_TYPE::serialize<boost::archive::text_iarchive>( \
			boost::archive::text_iarchive& ar, \
			const unsigned int file_version); \
		template void NAMESPACE_TYPE::serialize<boost::archive::text_oarchive>( \
			boost::archive::text_oarchive& ar, \
			const unsigned int file_version); \
		BOOST_CLASS_EXPORT_IMPLEMENT(NAMESPACE_TYPE) \

// serialization macros
#define _ARTAG(TYPENAME) ar & TYPENAME;
// serialization macros, requires if object is inherited from another object
#define _BSTSUPER(BASE) boost::serialization::base_object<BASE>(*this)

// part of serialization access implementation, serialize function implementation
#define SERIALIZER_ACCESS_IMPL2(NAMESPACE_TYPE, ...) \
		template <class Archive> \
		void NAMESPACE_TYPE::serialize(Archive& ar, const unsigned int file_version) \
		{ \
			__VA_ARGS__ \
		} \

// full serialization access implementation for a class only in the cpp file, requires a boost include
#define SERIALIZER_ACCESS_IMPL(NAMESPACE_TYPE, ...) \
		SERIALIZER_ACCESS_IMPL1(NAMESPACE_TYPE) \
		SERIALIZER_ACCESS_IMPL2(NAMESPACE_TYPE, __VA_ARGS__) \

#define CLIENT_OBJECT_IMPL(NAMESPACE_TYPE) \
		SERIALIZER_ACCESS_IMPL(NAMESPACE_TYPE, \
			_ARTAG(_BSTSUPER(Engine::Abstract::Object))) \

#define CLIENT_OBJECT_HEADER_DECLARATION(NAMESPACE, NAME) \
	namespace NAMESPACE { \
		class NAME : public Engine::Abstract::Object \
		{ \
		public: \
			NAME(); \
			~NAME() override; \
			void Initialize() override; \
			void PreUpdate(const float& dt) override; \
			void Update(const float& dt) override; \
			void PreRender(const float& dt) override; \
			void Render(const float& dt) override; \
		private: \
			SERIALIZER_ACCESS \
		} \
	} \
	BOOST_CLASS_EXPORT_KEY(NAMESPACE::NAME) \

namespace Engine
{
	namespace Objects
	{
		class Light;
		class Camera;
		class Text;
		class DebugObject;
	}

	namespace Component
	{
		class Collider;
		class Transform;
		class Rigidbody;
	}

	class Scene;
	class Layer;
	class Serializer;

	namespace Graphic
	{
		class IShader;
		class VertexShader;
	}

	namespace Resources
	{
		class Font;
		class Mesh;
		class Sound;
		class Texture;
		class NormalMap;
	}

	namespace Abstract
	{
		class Entity;
		class Object;
		class Component;
		class Actor;
		class Renderable;
		class Resource;
	}

	namespace Manager
	{
		class SceneManager;
		class ResourceManager;
		class D3Device;
		class ProjectionFrustum;
		class RenderPipeline;
		class ToolkitAPI;
		class CollisionManager;
		class PhysicsManager;
		class ConstraintSolver;
		class Debugger;
		class TaskScheduler;
		class Application;
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
	using ComponentID = LONG_PTR;
	using ActorID = LONG_PTR;

	using TaskSchedulerFunc = std::function<void(const float&)>;
}
