#pragma once
#include <atomic>
#include <map>
#include <unordered_map>
#include <string>
#include <set>
#include <vector>
#include <exception>
#include <fmod_common.h>
#include <typeindex>
#include <execution>
#include <algorithm>
#include <string>
#include <execution>
#include <memory>
#include <functional>
#include <ranges>

#include <wrl/client.h>

#include <SimpleMath.h>
#include <DirectXCollision.h>

#include <boost/make_shared.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/weak_ptr.hpp>

#include "egType.hpp"
#include "egSerialization.hpp"

using namespace DirectX::SimpleMath;
using namespace Microsoft::WRL;

namespace Engine
{
	extern Manager::ResourceManager& GetResourceManager();
	extern Manager::SceneManager& GetSceneManager();
	extern Manager::ProjectionFrustum& GetProjectionFrustum();
	extern Manager::CollisionDetector& GetCollisionDetector();
	extern Manager::Application& GetApplication();
	extern Manager::Graphics::D3Device& GetD3Device();
	extern Manager::Graphics::RenderPipeline& GetRenderPipeline();
	extern Manager::Graphics::ToolkitAPI& GetToolkitAPI();
	extern Manager::Physics::LerpManager& GetLerpManager();
	extern Manager::Physics::PhysicsManager& GetPhysicsManager();
	extern Manager::Physics::ConstraintSolver& GetConstraintSolver();
	extern Manager::Debugger& GetDebugger();
	extern Manager::TaskScheduler& GetTaskScheduler();
	extern Manager::MouseManager& GetMouseManager();
	
	constexpr float g_epsilon = 0.001f;
	constexpr float g_gravity_acc = 9.8f;

	constexpr float g_fixed_update_interval = 0.02f;
	constexpr int g_debug_y_movement = 15;
	constexpr int g_debug_y_initial = 0;
	constexpr float g_debug_message_life_time = 1.0f;
	constexpr size_t g_debug_message_max = 20;

	constexpr bool g_speculation_enabled = false;

	constexpr LONG_PTR g_invalid_id = -1;

	constexpr DirectX::SimpleMath::Vector3 g_forward = {0.f, 0.f, 1.f};
	constexpr DirectX::SimpleMath::Vector3 g_backward = {0.f, 0.f, -1.f};
	
	constexpr size_t g_max_map_size = 2048; // only in power of 2
	constexpr size_t g_octree_negative_round_up = g_max_map_size / 2;

	inline std::atomic<UINT> g_collision_energy_reduction_multiplier = 2;

	inline std::atomic<float> g_fov = DirectX::XM_PI / 4.f;
	inline std::atomic<bool> g_full_screen = false;
	inline std::atomic<bool> g_vsync_enabled = true;
	inline std::atomic<UINT> g_window_width = 1920;
	inline std::atomic<UINT> g_window_height = 1080;

	inline std::atomic<float> g_screen_near = 0.0001f;
	inline std::atomic<float> g_screen_far = 1000.0f;

	enum eShaderType
	{
		SHADER_VERTEX = 0,
		SHADER_PIXEL,
		SHADER_GEOMETRY,
		SHADER_COMPUTE,
		SHADER_HULL,
		SHADER_DOMAIN,
		SHADER_UNKNOWN
	};

	enum eCBType
	{
		CB_TYPE_WVP = 0,
		CB_TYPE_TRANSFORM,
		CB_TYPE_LIGHT,
		CB_TYPE_SPECULAR,
		CB_TYPE_SHADOW
	};

	enum eLayerType
	{
		LAYER_NONE = 0,
		LAYER_LIGHT,
		LAYER_DEFAULT,
		LAYER_ENVIRONMENT,
		LAYER_SKYBOX,
		LAYER_UI,
		LAYER_CAMERA,
		LAYER_MAX
	};

	enum eObserverState
	{
		OBSERVER_STATE_NONE,
	};

	enum eResourcePriority
	{
		RESOURCE_PRIORITY_SHADER = 0,
		RESOURCE_PRIORITY_TEXTURE,
		RESOURCE_PRIORITY_MESH,
		RESOURCE_PRIORITY_FONT,
		RESOURCE_PRIORITY_SOUND,
	};

	enum eComponentPriority
	{
		COMPONENT_PRIORITY_DEFAULT = 0,
		COMPONENT_PRIORITY_TRANSFORM,
		COMPONENT_PRIORITY_COLLIDER,
		COMPONENT_PRIORITY_RIGIDBODY,
		COMPONENT_PRIORITY_STATE
	};

	enum eBoundingType
	{
		BOUNDING_TYPE_BOX = 0,
		BOUNDING_TYPE_FRUSTUM,
		BOUNDING_TYPE_SPHERE,
	};

	enum eShaderResource
	{
		SR_TEXTURE = 0,
		SR_NORMAL_MAP,
		SR_SHADOW_MAP,
	};

	struct GUIDComparer
	{
		bool operator()(const GUID& Left, const GUID& Right) const
		{
			return memcmp(&Left, &Right, sizeof(Right)) < 0;
		}
	};

	struct WeakObjComparer
	{
		bool operator()(const WeakObject& lhs, const WeakObject& rhs) const
		{
			return lhs.lock().get() < rhs.lock().get();
		}
	};

	template <typename T>
	struct WeakComparer
	{
		bool operator()(const boost::weak_ptr<T>& lhs, const boost::weak_ptr<T>& rhs) const
		{
			return lhs.lock().get() < rhs.lock().get();
		}
	};

	struct ResourcePriorityComparer
	{
		bool operator()(const StrongResource& Left, const StrongResource& Right) const;
	};

	struct ComponentPriorityComparer
	{
		bool operator()(const WeakComponent& Left, const WeakComponent& Right) const;
	};

	union BoundingGroup
	{
		DirectX::BoundingOrientedBox box;
		DirectX::BoundingSphere sphere;
	};
}

namespace DX
{
	// Helper class for COM exceptions
	class com_exception : public std::exception
	{
	public:
		com_exception(HRESULT hr) : result(hr)
		{
		}

		const char* what() const noexcept override
		{
			static char s_str[64] = {};
			sprintf_s(s_str, "Failure with HRESULT of %08X",
			          static_cast<unsigned int>(result));
			return s_str;
		}

	private:
		HRESULT result;
	};

	// Helper utility converts D3D API failures into exceptions.
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			throw com_exception(hr);
		}
	}
}

namespace FMOD::DX
{
	// Helper class for COM exceptions
	class fmod_exception : public std::exception
	{
	public:
		fmod_exception(FMOD_RESULT hr) : result(hr)
		{
		}

		const char* what() const noexcept override
		{
			static char s_str[64] = {};
			sprintf_s(s_str, "Failure with FMOD_RESULT of %08X",
			          static_cast<unsigned int>(result));
			return s_str;
		}

	private:
		HRESULT result;
	};

	// Helper utility converts D3D API failures into exceptions.
	inline void ThrowIfFailed(FMOD_RESULT hr)
	{
		if (hr != FMOD_OK)
		{
			throw fmod_exception(hr);
		}
	}
}