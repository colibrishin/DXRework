#pragma once
#include <atomic>
#include <map>
#include <unordered_map>
#include <string>
#include <exception>
#include <fmod_common.h>
#include <typeindex>
#include <SimpleMath.h>

#include "egType.hpp"

namespace Engine
{
	namespace Manager
	{
		namespace Graphics
		{
			class ToolkitAPI;
		}

		class Application;
		class ResourceManager;
		class SceneManager;
		class ProjectionFrustum;
		class CollisionDetector;
	}

	namespace Abstract
	{
		class Object;
	}

	extern Manager::Application& GetApplication();
	extern Manager::ResourceManager& GetResourceManager();
	extern Manager::SceneManager& GetSceneManager();
	extern Manager::ProjectionFrustum& GetProjectionFrustum();
	extern Manager::CollisionDetector& GetCollisionDetector();

	constexpr float g_epsilon = 0.001f;
	constexpr float g_gravity_acc = 9.8f;
	constexpr int g_max_lights = 8;

	constexpr float g_fixed_update_interval = 0.02f;
	constexpr int g_debug_y_movement = 15;
	constexpr int g_debug_y_initial = 0;
	constexpr float g_debug_message_life_time = 1.0f;
	constexpr size_t g_debug_message_max = 20;

	constexpr DirectX::SimpleMath::Vector3 g_forward = {0.f, 0.f, 1.f};
	constexpr DirectX::SimpleMath::Vector3 g_backward = {0.f, 0.f, -1.f};
	
	constexpr size_t g_max_map_size = 2048; // only in power of 2
	constexpr size_t g_octree_negative_round_up = g_max_map_size / 2;

	inline std::atomic<bool> g_full_screen = false;
	inline std::atomic<bool> g_vsync_enabled = true;
	inline std::atomic<UINT> g_window_width = 800;
	inline std::atomic<UINT> g_window_height = 600;

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
		CB_TYPE_VP = 0,
		CB_TYPE_TRANSFORM,
		CB_TYPE_LIGHT_POSITION,
		CB_TYPE_LIGHT_COLOR,
		CB_TYPE_SPECULAR
	};

	enum eLayerType
	{
		LAYER_NONE = 0,
		LAYER_LIGHT,
		LAYER_DEFAULT,
		LAYER_UI,
		LAYER_CAMERA,
		LAYER_MAX
	};
	
	constexpr eLayerType g_early_update_layer_end = LAYER_DEFAULT;

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
		bool operator()(const std::weak_ptr<Abstract::Object>& lhs, const std::weak_ptr<Abstract::Object>& rhs) const
		{
			return lhs.lock().get() < rhs.lock().get();
		}
	};

	template <typename T>
	struct WeakComparer
	{
		bool operator()(const std::weak_ptr<T>& lhs, const std::weak_ptr<T>& rhs) const
		{
			return lhs.lock().get() < rhs.lock().get();
		}
	};

	struct ResourcePriorityComparer
	{
		bool operator()(const WeakResource& Left, const WeakResource& Right) const;
	};

	struct ComponentPriorityComparer
	{
		bool operator()(const WeakComponent& Left, const WeakComponent& Right) const;
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