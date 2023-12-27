#pragma once
#include <Windows.h>
#include <functional>
#include <map>
#include <memory>
#include <boost/smart_ptr/weak_ptr.hpp>

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
        class SoundPlayer;
        class ModelRenderer;
        class Animator;
    } // namespace Component

    class Scene;
    class Layer;
    class Serializer;

    namespace Graphics
    {
        template <typename T>
        class Shader;
        class IShader;
        class VertexShaderInternal;

        struct AnimationPrimitive;
        struct BonePrimitive;
        struct BoneAnimationPrimitive;
    } // namespace Graphic

    namespace Resources
    {
        class Font;
        class Mesh;
        class Sound;
        class Texture;
        class NormalMap;
        class BoneAnimation;
        class Model;
        class Bone;
        class BaseAnimation;

        using VertexShader = Graphics::VertexShaderInternal;
        using PixelShader = Graphics::Shader<ID3D11PixelShader>;
        using GeometryShader = Graphics::Shader<ID3D11GeometryShader>;
        using HullShader = Graphics::Shader<ID3D11HullShader>;
        using DomainShader = Graphics::Shader<ID3D11DomainShader>;
        using ComputeShader = Graphics::Shader<ID3D11ComputeShader>;
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
            class ReflectionEvaluator;
            class Renderer;
        } // namespace Graphics

        namespace Physics
        {
            class PhysicsManager;
            class LerpManager;
            class ConstraintSolver;
            class CollisionDetector;
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
    using WeakBoneAnimation = boost::weak_ptr<Resources::BoneAnimation>;
    using WeakModel = boost::weak_ptr<Resources::Model>;
    using WeakBone = boost::weak_ptr<Resources::Bone>;
    using WeakTransform = boost::weak_ptr<Components::Transform>;
    using WeakModelRenderer = boost::weak_ptr<Components::ModelRenderer>;
    using WeakAnimator = boost::weak_ptr<Components::Animator>;
    using WeakBaseAnimation = boost::weak_ptr<Resources::BaseAnimation>;
    using WeakVertexShader = boost::weak_ptr<Resources::VertexShader>;
    using WeakPixelShader = boost::weak_ptr<Resources::PixelShader>;

    // Strong pointer type definitions
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
    using StrongBoneAnimation = boost::shared_ptr<Resources::BoneAnimation>;
    using StrongModel = boost::shared_ptr<Resources::Model>;
    using StrongBone = boost::shared_ptr<Resources::Bone>;
    using StrongBaseAnimation = boost::shared_ptr<Resources::BaseAnimation>;
    using StrongTransform = boost::shared_ptr<Components::Transform>;
    using StrongVertexShader = boost::shared_ptr<Resources::VertexShader>;
    using StrongPixelShader = boost::shared_ptr<Resources::PixelShader>;

    // Misc type definitions
    using BonePrimitiveMap = std::map<std::string, Graphics::BonePrimitive>;
    using EntityID = LONG_PTR;
    using ComponentID = LONG_PTR;
    using ActorID = LONG_PTR;
    using EntityName = std::string;
    using TypeName = std::string;
    using TaskSchedulerFunc = std::function<void(const float&)>;

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
    struct which_scene
    {
        static constexpr eSceneType value = T::stype;
    };

    template <typename T>
    struct which_cb
    {
        static constexpr eCBType value = T::cbtype;
    };

    struct GUIDComparer
    {
        bool operator()(const GUID& Left, const GUID& Right) const
        {
            return memcmp(&Left, &Right, sizeof(Right)) < 0;
        }
    };
} // namespace Engine
