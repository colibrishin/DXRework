#pragma once
#include <Windows.h>
#include <functional>
#include <map>
#include <memory>
#include <boost/smart_ptr/weak_ptr.hpp>

#include <DirectXCollision.h>
#include <SimpleMath.h>
#include "Effects.h"
#include "BufferHelpers.h"
#include <wrl/client.h>

#include <CommonStates.h>
#include <GeometricPrimitive.h>
#include <Keyboard.h>
#include <Mouse.h>
#include <SpriteBatch.h>
#include <SpriteFont.h>

#include <d3d11.h>

// Need to be included before boost only in the header, requires a default
// constructor
#define SERIALIZER_ACCESS                                                      \
  friend class Engine::Serializer;                                             \
  friend class boost::serialization::access;                                   \
  template <class Archive>                                                     \
  void serialize(Archive &ar, const unsigned int file_version);
// part of serialization access implementation, forward declaration of serialize
// function
#define SERIALIZER_ACCESS_IMPL1(NAMESPACE_TYPE)                                \
  template void NAMESPACE_TYPE::serialize<boost::archive::text_iarchive>(      \
      boost::archive::text_iarchive & ar, const unsigned int file_version);    \
  template void NAMESPACE_TYPE::serialize<boost::archive::text_oarchive>(      \
      boost::archive::text_oarchive & ar, const unsigned int file_version);    \
  BOOST_CLASS_EXPORT_IMPLEMENT(NAMESPACE_TYPE)
// serialization macros
#define _ARTAG(TYPENAME) ar & TYPENAME;
// serialization macros, requires if object is inherited from another object
#define _BSTSUPER(BASE) boost::serialization::base_object<BASE>(*this)

// part of serialization access implementation, serialize function
// implementation
#define SERIALIZER_ACCESS_IMPL2(NAMESPACE_TYPE, ...)                           \
  template <class Archive>                                                     \
  void NAMESPACE_TYPE::serialize(Archive &ar,                                  \
                                 const unsigned int file_version) {            \
    __VA_ARGS__                                                                \
  }
// full serialization access implementation for a class only in the cpp file,
// requires a boost include
#define SERIALIZER_ACCESS_IMPL(NAMESPACE_TYPE, ...)                            \
  SERIALIZER_ACCESS_IMPL1(NAMESPACE_TYPE)                                      \
  SERIALIZER_ACCESS_IMPL2(NAMESPACE_TYPE, __VA_ARGS__)

// Static Component type, this should be added to every component
#define INTERNAL_COMP_CHECK_CONSTEXPR(enum_val) static constexpr eComponentType ctype = enum_val;
// Static Resource type, this should be added to every resource
#define INTERNAL_RES_CHECK_CONSTEXPR(enum_val) static constexpr eResourceType rtype = enum_val;
// Static engine default provided object type, this should be added to every object
#define INTERNAL_OBJECT_CHECK_CONSTEXPR(enum_val) static constexpr eDefObjectType dotype = enum_val;

// Static client provided scene type, this should be added to every scene in the client
#define CLIENT_SCENE_CHECK_CONSTEXPR(enum_val) static constexpr Engine::eSceneType stype = enum_val;

#define RESOURCE_SELF_INFER_GETTER(TYPE)                                      \
  static inline boost::weak_ptr<TYPE> Get(const std::string& name)            \
    { return GetResourceManager().GetResource<TYPE>(name); }

#define RESOURCE_SELF_INFER_CREATE(TYPE)                                      \
    static inline boost::shared_ptr<TYPE> Create(                             \
    const std::string& name, const std::filesystem::path& path)               \
    {                                                                         \
        if (const auto check = GetResourceManager().                          \
                               GetResourceByPath<TYPE>(path).lock())          \
        {                                                                     \
            return check;                                                     \
        }                                                                     \
        const auto obj = boost::make_shared<TYPE>(path);                      \
        GetResourceManager().AddResource(name, obj);                          \
        return obj;                                                           \
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
    using DirectX::ConstantBuffer;

    using DirectX::BoundingBox;
    using DirectX::BoundingFrustum;
    using DirectX::BoundingOrientedBox;
    using DirectX::BoundingSphere;

    using Microsoft::WRL::ComPtr;

    using DirectX::XMFLOAT2;
    using DirectX::XMFLOAT3X3;
    using DirectX::XMVECTORF32;

    struct AnimationPrimitive;
    struct BonePrimitive;

    namespace Objects
    {
        class Light;
        class Camera;
        class Text;
        class DebugObject;
        class Observer;
    } // namespace Objects

    namespace Components
    {
        class Collider;
        class Transform;
        class Rigidbody;
        class ObserverController;
        class SoundPlayer;
    } // namespace Component

    class Scene;
    class Layer;
    class Serializer;

    namespace Graphic
    {
        template <typename T>
        class Shader;
        class IShader;
        class VertexShaderInternal;

        using VertexShader = VertexShaderInternal;
        using PixelShader = Shader<ID3D11PixelShader>;
        using GeometryShader = Shader<ID3D11GeometryShader>;
        using HullShader = Shader<ID3D11HullShader>;
        using DomainShader = Shader<ID3D11DomainShader>;
        using ComputeShader = Shader<ID3D11ComputeShader>;
    } // namespace Graphic

    namespace Resources
    {
        class Font;
        class Mesh;
        class Sound;
        class Texture;
        class NormalMap;
        class Animation;
        class Model;
        class Bone;
    } // namespace Resources

    namespace Abstract
    {
        class Entity;
        class Object;
        class Component;
        class Actor;
        class Renderable;
        class Resource;
        class IStateController;
    } // namespace Abstract

    namespace Manager
    {
        namespace Graphics
        {
            class ToolkitAPI;
            class RenderPipeline;
            class D3Device;
            class ShadowManager;
        } // namespace Graphics

        namespace Physics
        {
            class PhysicsManager;
            class LerpManager;
            class ConstraintSolver;
        } // namespace Physics

        class ProjectionFrustum;
        class Application;
        class ResourceManager;
        class SceneManager;
        class ProjectionFrustum;
        class CollisionDetector;
        class Debugger;
        class MouseManager;
        class Application;
        class TaskScheduler;
        class FBXLoader;
    } // namespace Manager

    using WeakObject = boost::weak_ptr<Abstract::Object>;
    using WeakComponent = boost::weak_ptr<Abstract::Component>;
    using WeakResource = boost::weak_ptr<Abstract::Resource>;
    using WeakMesh = boost::weak_ptr<Resources::Mesh>;
    using WeakScene = boost::weak_ptr<Scene>;
    using WeakCollider = boost::weak_ptr<Components::Collider>;
    using WeakFont = boost::weak_ptr<Resources::Font>;
    using WeakCamera = boost::weak_ptr<Objects::Camera>;
    using WeakLight = boost::weak_ptr<Objects::Light>;
    using WeakTexture = boost::weak_ptr<Resources::Texture>;
    using WeakNormalMap = boost::weak_ptr<Resources::NormalMap>;
    using WeakAnimation = boost::weak_ptr<Resources::Animation>;
    using WeakModel = boost::weak_ptr<Resources::Model>;

    using WeakVertexShader = boost::weak_ptr<Graphic::VertexShader>;
    using WeakPixelShader = boost::weak_ptr<Graphic::PixelShader>;

    using StrongObject = boost::shared_ptr<Abstract::Object>;
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
    using StrongNormalMap = boost::shared_ptr<Resources::NormalMap>;
    using StrongAnimation = boost::shared_ptr<Resources::Animation>;
    using StrongModel = boost::shared_ptr<Resources::Model>;
    using StrongBone = boost::shared_ptr<Resources::Bone>;

    using StrongVertexShader = boost::shared_ptr<Graphic::VertexShader>;
    using StrongPixelShader = boost::shared_ptr<Graphic::PixelShader>;

    using BonePrimitiveMap = std::map<std::string, BonePrimitive>;

    using EntityID = LONG_PTR;
    using ComponentID = LONG_PTR;
    using ActorID = LONG_PTR;
    using EntityName = std::string;
    using TypeName = std::string;

    using TaskSchedulerFunc = std::function<void(const float&)>;

    extern Manager::ResourceManager&           GetResourceManager();
    extern Manager::SceneManager&              GetSceneManager();
    extern Manager::ProjectionFrustum&         GetProjectionFrustum();
    extern Manager::CollisionDetector&         GetCollisionDetector();
    extern Manager::Application&               GetApplication();
    extern Manager::Graphics::D3Device&        GetD3Device();
    extern Manager::Graphics::RenderPipeline&  GetRenderPipeline();
    extern Manager::Graphics::ToolkitAPI&      GetToolkitAPI();
    extern Manager::Physics::LerpManager&      GetLerpManager();
    extern Manager::Physics::PhysicsManager&   GetPhysicsManager();
    extern Manager::Physics::ConstraintSolver& GetConstraintSolver();
    extern Manager::Debugger&                  GetDebugger();
    extern Manager::TaskScheduler&             GetTaskScheduler();
    extern Manager::MouseManager&              GetMouseManager();
    extern Manager::FBXLoader&                 GetFBXLoader();
} // namespace Engine
