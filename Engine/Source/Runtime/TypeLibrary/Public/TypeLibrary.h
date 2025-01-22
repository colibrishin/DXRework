#pragma once
#include <string>
#include <directxtk12/SimpleMath.h>
#include <DirectXMath.h>

using Vector2 = DirectX::SimpleMath::Vector2;
using Vector3 = DirectX::SimpleMath::Vector3;
using Vector4 = DirectX::SimpleMath::Vector4;
using Color = DirectX::SimpleMath::Color;
using Quaternion = DirectX::SimpleMath::Quaternion;
using Ray = DirectX::SimpleMath::Ray;
using Matrix = DirectX::SimpleMath::Matrix;

namespace Microsoft::WRL
{
	template <typename T>
	class ComPtr;
}

namespace Engine
{
	template <typename T>
	using Weak = boost::weak_ptr<T>;

	template <typename T>
	using Strong = boost::shared_ptr<T>;

	using GenericString = std::string;
	using EntityName = GenericString;
	using TypeName = GenericString;

	using IDType = uint32_t;
	using GlobalEntityID = IDType;
	using LocalActorID = IDType;
	using LocalComponentID = IDType;

	using LayerSizeType = uint32_t;
	using ScriptSizeType = uint32_t;

	inline constexpr static IDType g_invalid_id = -1;

    using UINT = uint32_t;

	enum eResourceType;
	enum eComponentType;
	enum eDefObjectType;
	enum eTaskType;

	using DirectX::BoundingBox;
	using DirectX::BoundingFrustum;
	using DirectX::BoundingOrientedBox;
	using DirectX::BoundingSphere;

	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	using DirectX::XMFLOAT2;
	using DirectX::XMFLOAT3X3;
	using DirectX::XMVECTORF32;

	using DirectX::CommonStates;
	using DirectX::GeometricPrimitive;
	using DirectX::Keyboard;
	using DirectX::Mouse;
	using DirectX::SpriteBatch;
	using DirectX::SpriteFont;
	using DirectX::VertexPositionColor;
	using DirectX::BasicEffect;

	struct CommandPair;
	struct DescriptorHandler;
	struct DescriptorPtrImpl;

	using StrongDescriptorPtr = boost::shared_ptr<DescriptorPtrImpl>;
	using DescriptorPtr = boost::weak_ptr<DescriptorPtrImpl>;

	class Serializer;
	struct ComponentPriorityComparer;

	template <typename WeakT, typename BoundingValueGetter, float Epsilon>
	class Octree;

	namespace Objects
	{
		class Light;
		class Camera;
		class Text;
		class Observer;
	} // namespace Objects

	namespace Components
	{
		namespace Abstracts
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

	using VertexCollection = std::vector<Graphics::VertexElement>;

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

	namespace Abstracts
	{
		class Entity;
		class ObjectBase;
		class Component;
		class Actor;
		class Renderable;
		class Resource;

		template <typename T, typename... Args>
		class Singleton;
	} // namespace Abstracts

	namespace Managers
	{
		namespace Graphics
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
	} // namespace Managers
}
