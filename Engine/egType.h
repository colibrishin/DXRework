#pragma once
#include <Windows.h>
#include <functional>
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

namespace Engine
{
    struct Joint;
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

    using StrongObject = boost::shared_ptr<Abstract::Object>;
    using StrongComponent = boost::shared_ptr<Abstract::Component>;
    using StrongResource = boost::shared_ptr<Abstract::Resource>;
    using StrongScene = boost::shared_ptr<Scene>;
    using StrongLight = boost::shared_ptr<Objects::Light>;
    using StrongLayer = boost::shared_ptr<Layer>;
    using StrongCamera = boost::shared_ptr<Objects::Camera>;
    using StrongFont = boost::shared_ptr<Resources::Font>;
    using StrongMesh = boost::shared_ptr<Resources::Mesh>;

    using JointMap = std::map<std::string, Joint>;

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
