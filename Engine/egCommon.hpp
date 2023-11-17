#pragma once
#include <atomic>
#include <map>
#include <unordered_map>
#include <string>
#include <exception>
#include <typeindex>

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
	constexpr float g_fixed_update_interval = 0.07f;

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
		LAYER_LIGHT = 0,
		LAYER_CAMERA,
		LAYER_DEFAULT,
		LAYER_UI,
		LAYER_MAX
	};

	enum eResourcePriority
	{
		RESOURCE_PRIORITY_SHADER = 0,
		RESOURCE_PRIORITY_TEXTURE,
		RESOURCE_PRIORITY_MESH,
		RESOURCE_PRIORITY_FONT
	};

	enum eComponentPriority
	{
		COMPONENT_PRIORITY_DEFAULT = 0,
		COMPONENT_PRIORITY_TRANSFORM,
		COMPONENT_PRIORITY_COLLIDER,
		COMPONENT_PRIORITY_RIGIDBODY
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
