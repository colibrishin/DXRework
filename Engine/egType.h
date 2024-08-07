#pragma once
#include <Windows.h>
#include <any>
#include <functional>
#include <map>
#include <memory>
#include <type_traits>
#include <boost/smart_ptr/weak_ptr.hpp>
#include <oneapi/tbb.h>

#include "egEnums.h"

namespace ImGui
{
	extern void Text(const char* fmt, ...);
}

namespace Engine
{
	using DirectX::SimpleMath::Color;
	using DirectX::SimpleMath::Matrix;
	using DirectX::SimpleMath::Quaternion;
	using DirectX::SimpleMath::Ray;
	using DirectX::SimpleMath::Vector2;
	using DirectX::SimpleMath::Vector3;
	using DirectX::SimpleMath::Vector4;

	using DirectX::CommonStates;
	using DirectX::GeometricPrimitive;
	using DirectX::Keyboard;
	using DirectX::Mouse;
	using DirectX::SpriteBatch;
	using DirectX::SpriteFont;
	using DirectX::VertexPositionColor;
	using DirectX::BasicEffect;

	using DirectX::BoundingBox;
	using DirectX::BoundingFrustum;
	using DirectX::BoundingOrientedBox;
	using DirectX::BoundingSphere;

	using tbb::concurrent_hash_map;
	using tbb::concurrent_map;
	using tbb::concurrent_queue;
	using tbb::concurrent_vector;
	using tbb::concurrent_set;

	using Microsoft::WRL::ComPtr;

	using DirectX::XMFLOAT2;
	using DirectX::XMFLOAT3X3;
	using DirectX::XMVECTORF32;

	struct CommandPair;
	struct DescriptorHandler;
	struct DescriptorPtrImpl;

	using StrongDescriptorPtr = boost::shared_ptr<DescriptorPtrImpl>;
	using DescriptorPtr = boost::weak_ptr<DescriptorPtrImpl>;

	namespace Objects
	{
		class Light;
		class Camera;
		class Text;
		class Observer;
	} // namespace Objects

	namespace Components { namespace Base
		{
			class RenderComponent;
		}

		class Collider;
		class OffsetCollider;
		class Transform;
		class Rigidbody;
		class ObserverController;
		class SoundPlayer;
		class ModelRenderer;
		class Animator;
		class ParticleRenderer;
	} // namespace Component

	class Script;
	class Scene;
	class Layer;
	class Serializer;
	struct ComponentPriorityComparer;

	namespace Graphics
	{
		template <typename T>
		class StructuredBuffer;

		struct AnimationPrimitive;
		struct BonePrimitive;
		struct BoneAnimationPrimitive;
		struct VertexElement;

		namespace SBs
		{
			struct InstanceSB;
			struct InstanceParticleSB;
			struct LocalParamSB;
			struct InstanceModelSB;
			struct MaterialSB;
		} // namespace SBs
	} // namespace Graphic

	namespace Resources
	{
		class Prefab;
		class Font;
		class Mesh;
		class Sound;
		class Texture;
		class BoneAnimation;
		class Shape;
		class Bone;
		class BaseAnimation;
		class Material;
		class Shader;
		class AnimationsTexture;
		class ShadowTexture;
		class Texture1D;
		class Texture2D;
		class Texture3D;
		class ComputeShader;
	} // namespace Resources

	namespace Abstract
	{
		class Entity;
		class ObjectBase;
		class Component;
		class Actor;
		class Renderable;
		class Resource;
		class IStateController;

		template <typename T, typename... Args>
		class Singleton;
	} // namespace Abstract

	namespace Manager { namespace Graphics
		{
			class RaytracingPipeline;
			class Raytracer;
			class ToolkitAPI;
			class RenderPipeline;
			class D3Device;
			class ShadowManager;
			class ReflectionEvaluator;
			class Renderer;
			class ImGuiManager;
		} // namespace Graphics

		namespace Physics
		{
			class PhysicsManager;
			class LerpManager;
			class ConstraintSolver;
			class CollisionDetector;
			class Graviton;
		} // namespace Physics

		class ProjectionFrustum;
		class Application;
		class ResourceManager;
		class SceneManager;
		class Debugger;
		class MouseManager;
		class TaskScheduler;
	} // namespace Manager

	// Weak pointer type definitions
	using WeakObjectBase = boost::weak_ptr<Abstract::ObjectBase>;
	using WeakComponent = boost::weak_ptr<Abstract::Component>;
	using WeakResource = boost::weak_ptr<Abstract::Resource>;
	using WeakMesh = boost::weak_ptr<Resources::Mesh>;
	using WeakScene = boost::weak_ptr<Scene>;
	using WeakCollider = boost::weak_ptr<Components::Collider>;
	using WeakFont = boost::weak_ptr<Resources::Font>;
	using WeakCamera = boost::weak_ptr<Objects::Camera>;
	using WeakLight = boost::weak_ptr<Objects::Light>;
	using WeakTexture = boost::weak_ptr<Resources::Texture>;
	using WeakBoneAnimation = boost::weak_ptr<Resources::BoneAnimation>;
	using WeakModel = boost::weak_ptr<Resources::Shape>;
	using WeakBone = boost::weak_ptr<Resources::Bone>;
	using WeakTransform = boost::weak_ptr<Components::Transform>;
	using WeakModelRenderer = boost::weak_ptr<Components::ModelRenderer>;
	using WeakAnimator = boost::weak_ptr<Components::Animator>;
	using WeakBaseAnimation = boost::weak_ptr<Resources::BaseAnimation>;
	using WeakMaterial = boost::weak_ptr<Resources::Material>;
	using WeakScript = boost::weak_ptr<Script>;
	using WeakAnimsTexture = boost::weak_ptr<Resources::AnimationsTexture>;
	using WeakShadowTexture = boost::weak_ptr<Resources::ShadowTexture>;
	using WeakParticleRenderer = boost::weak_ptr<Components::ParticleRenderer>;
	using WeakComputeShader = boost::weak_ptr<Resources::ComputeShader>;
	using WeakTexture2D = boost::weak_ptr<Resources::Texture2D>;
	using WeakPrefab = boost::weak_ptr<Resources::Prefab>;

	template <typename T>
	using Weak = boost::weak_ptr<T>;

	// Strong pointer type definitions
	using StrongObjectBase = boost::shared_ptr<Abstract::ObjectBase>;
	using StrongComponent = boost::shared_ptr<Abstract::Component>;
	using StrongResource = boost::shared_ptr<Abstract::Resource>;
	using StrongScene = boost::shared_ptr<Scene>;
	using StrongLight = boost::shared_ptr<Objects::Light>;
	using StrongLayer = boost::shared_ptr<Layer>;
	using StrongCamera = boost::shared_ptr<Objects::Camera>;
	using StrongFont = boost::shared_ptr<Resources::Font>;
	using StrongMesh = boost::shared_ptr<Resources::Mesh>;
	using StrongSound = boost::shared_ptr<Resources::Sound>;
	using StrongTexture = boost::shared_ptr<Resources::Texture>;
	using StrongBoneAnimation = boost::shared_ptr<Resources::BoneAnimation>;
	using StrongModel = boost::shared_ptr<Resources::Shape>;
	using StrongBone = boost::shared_ptr<Resources::Bone>;
	using StrongBaseAnimation = boost::shared_ptr<Resources::BaseAnimation>;
	using StrongTransform = boost::shared_ptr<Components::Transform>;
	using StrongShader = boost::shared_ptr<Resources::Shader>;
	using StrongMaterial = boost::shared_ptr<Resources::Material>;
	using StrongCollider = boost::shared_ptr<Components::Collider>;
	using StrongScript = boost::shared_ptr<Script>;
	using StrongAnimsTexture = boost::shared_ptr<Resources::AnimationsTexture>;
	using StrongShadowTexture = boost::shared_ptr<Resources::ShadowTexture>;
	using StrongTexture2D = boost::shared_ptr<Resources::Texture2D>;
	using StrongModelRenderer = boost::shared_ptr<Components::ModelRenderer>;
	using StrongRenderComponent = boost::shared_ptr<Components::Base::RenderComponent>;
	using StrongParticleRenderer = boost::shared_ptr<Components::ParticleRenderer>;
	using StrongComputeShader = boost::shared_ptr<Resources::ComputeShader>;
	using StrongPrefab = boost::shared_ptr<Resources::Prefab>;
	using StrongRigidbody = boost::shared_ptr<Components::Rigidbody>;

	template <typename T>
	using Strong = boost::shared_ptr<T>;

	// Misc type definitions
	using BonePrimitiveMap = std::map<std::string, Graphics::BonePrimitive>;
	using GlobalEntityID = LONG_PTR;
	using LocalComponentID = LONG_PTR;
	using LocalActorID = LONG_PTR;
	using LocalResourceID = LONG_PTR;
	using EntityName = std::string;
	using TypeName = std::string;
	using MetadataPathStr = std::string;
	using RawPathStr = std::string;
	using TaskSchedulerFunc = std::function<void(const std::vector<std::any>&, float)>;
	using VertexCollection = std::vector<Graphics::VertexElement>;
	using IndexCollection = std::vector<UINT>;
	using VertexBufferCollection = std::vector<ComPtr<ID3D12Resource>>;
	using IndexBufferCollection = std::vector<ComPtr<ID3D12Resource>>;
	using InstanceParticles = std::vector<Graphics::SBs::InstanceParticleSB>;

	// Concurrent type definitions
	using ConcurrentWeakObjGlobalMap = concurrent_hash_map<GlobalEntityID, WeakObjectBase>;
	using ConcurrentWeakObjVec = concurrent_vector<WeakObjectBase>;
	using ConcurrentLocalGlobalIDMap = concurrent_hash_map<LocalActorID, GlobalEntityID>;
	using ConcurrentWeakComVec = concurrent_vector<WeakComponent>;
	using ConcurrentWeakComMap = concurrent_hash_map<GlobalEntityID, WeakComponent>;
	using ConcurrentWeakScpVec = concurrent_vector<WeakScript>;
	using ConcurrentWeakScpMap = concurrent_hash_map<GlobalEntityID, WeakScript>;
	using ConcurrentWeakComRootMap = concurrent_hash_map<eComponentType, ConcurrentWeakComMap>;
	using ConcurrentWeakScpRootMap = concurrent_hash_map<eScriptType, ConcurrentWeakScpMap>;
	using ConcurrentVector3Vec = concurrent_vector<Vector3>;

	// Bitwise Enums
	using eShaderRasterizers = UINT;
	using eShaderDepths = UINT;
	using eShaderSamplers = UINT;
	using eTexBindSlots = UINT;

	// Manager Forward Declaration
	extern Manager::ResourceManager&               GetResourceManager();
	extern Manager::SceneManager&                  GetSceneManager();
	extern Manager::ProjectionFrustum&             GetProjectionFrustum();
	extern Manager::Physics::CollisionDetector&    GetCollisionDetector();
	extern Manager::Application&                   GetApplication();
	extern Manager::Graphics::D3Device&            GetD3Device();
	extern Manager::Graphics::RenderPipeline&      GetRenderPipeline();
	extern Manager::Graphics::ToolkitAPI&          GetToolkitAPI();
	extern Manager::Physics::LerpManager&          GetLerpManager();
	extern Manager::Physics::PhysicsManager&       GetPhysicsManager();
	extern Manager::Physics::ConstraintSolver&     GetConstraintSolver();
	extern Manager::Debugger&                      GetDebugger();
	extern Manager::TaskScheduler&                 GetTaskScheduler();
	extern Manager::MouseManager&                  GetMouseManager();
	extern Manager::Graphics::ReflectionEvaluator& GetReflectionEvaluator();
	extern Manager::Graphics::ShadowManager&       GetShadowManager();
	extern Manager::Graphics::Renderer&            GetRenderer();
	extern Manager::Graphics::ImGuiManager&        GetImGuiManager();
	extern Manager::Graphics::RaytracingPipeline&  GetRaytracingPipeline();
	extern Manager::Graphics::Raytracer&           GetRaytracer();

	using FrameIndex = UINT64;

	using DescriptorContainer = concurrent_vector<StrongDescriptorPtr>;
	using InstanceBufferContainer = concurrent_vector<Graphics::StructuredBuffer<Graphics::SBs::InstanceSB>>;
	using ObjectPredication = std::function<bool(const StrongObjectBase&)>;
	using CommandDescriptorLambda = std::function<void(const Weak<CommandPair>&, const DescriptorPtr&)>;

	// Unwrapping template type (e.g., std::shared_ptr<T> -> T)
	template <typename WrapT>
	struct unwrap;

	template <template <typename> class WrapT, typename T>
	struct unwrap<WrapT<T>>
	{
		using type = T;
	};

	// Getting wrapper type (e.g., std::shared_ptr<T> -> std::shared_ptr<void>)
	template <typename WrapT>
	struct get_wrapper;

	template <template <typename> class WrapT, typename T>
	struct get_wrapper<WrapT<T>>
	{
		using type = WrapT<void>;
	};

	// Static type checkers
	template <typename T>
	struct which_resource
	{
		static constexpr eResourceType value = T::rtype;
	};

	template <typename T>
	struct which_component
	{
		static constexpr eComponentType value = T::ctype;
	};

	template <typename T>
	struct which_def_object
	{
		static constexpr eDefObjectType value = T::dotype;
	};

	template <typename T>
	struct which_cb
	{
		static constexpr eCBType value = T::cbtype;
	};

	template <typename T>
	struct which_sb
	{
		static constexpr eSBType value = T::sbtype;
	};

	template <typename T>
	struct which_client_sb
	{
		static constexpr eClientSBType value = T::csbtype;
	};

	template <typename T>
	struct which_script
	{
		static constexpr eScriptType value = T::scptype;
	};

	template <typename T>
	struct which_sb_uav
	{
		static constexpr eSBUAVType value = T::sbuavtype;
	};

	template <typename T>
	struct which_client_sb_uav
	{
		static constexpr eClientSBUAVType value = T::csbuavtype;
	};


	template <typename... T>
	struct void_stub
	{
		using type = void;
	};

	template <typename... T>
	using void_t = typename void_stub<T...>::type;

	// Structured buffer type checkers. is_uav_sb<T> will have "::type" if T has sbuavtype as member.
	// if not then it will have no "::type".
	template <typename T, typename = void>
	struct is_uav_sb : std::false_type {};

	template <typename T>
	struct is_uav_sb<T, void_t<decltype(T::sbuavtype == true)>> : std::true_type {};

	template <typename T>
	struct is_uav_sb<T, void_t<decltype(T::csbuavtype == true)>> : std::true_type {};

	template <typename T, typename = void>
	struct is_client_uav_sb : std::false_type {};

	template <typename T>
	struct is_client_uav_sb<T, void_t<decltype(T::csbuavtype == true)>> : std::true_type {};

	template <typename T, typename = void>
	struct is_sb : std::false_type {};

	template <typename T>
	struct is_sb<T, void_t<decltype(T::sbtype == true)>> : std::true_type {};

	template <typename T, typename = void>
	struct is_client_sb : std::false_type {};

	template <typename T>
	struct is_client_sb<T, void_t<decltype(T::csbtype == true)>> : std::true_type {};

	struct GUIDComparer
	{
		bool operator()(const GUID& Left, const GUID& Right) const
		{
			return memcmp(&Left, &Right, sizeof(Right)) < 0;
		}
	};
} // namespace Engine
