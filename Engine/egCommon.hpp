#pragma once
#include <map>
#include <unordered_map>
#include <SimpleMath.h>
#include <string>
#include <d3d11.h>
#include <exception>
#include <typeindex>

#include "egApplication.hpp"

using namespace DirectX::SimpleMath;

namespace Engine
{
	namespace Abstract
	{
		class Object;
	}

	constexpr float g_gravity_acc = 9.8f;
	constexpr int g_max_lights = 8;

	struct VertexElement
	{
		Vector3 position;
		Vector3 normal;
		Vector4 color;
		Vector2 texCoord;
		Vector3 tangent;
		Vector3 binormal;
	};

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
		COMPONENT_PRIORITY_RIGIDBODY,
		COMPONENT_PRIORITY_COLLIDER
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

	union BoundingGroup
	{
		BoundingOrientedBox box;
		BoundingSphere sphere;
		BoundingFrustum frustum;
	};

	struct GUIDComparer
	{
		bool operator()(const GUID& Left, const GUID& Right) const
		{
			return memcmp(&Left, &Right, sizeof(Right)) < 0;
		}
	};

	const std::map<GUID, eShaderType, GUIDComparer> g_shader_enum_type_map =
	{
		{__uuidof(ID3D11VertexShader), SHADER_VERTEX},
		{__uuidof(ID3D11PixelShader), SHADER_PIXEL},
		{__uuidof(ID3D11GeometryShader), SHADER_GEOMETRY},
		{__uuidof(ID3D11ComputeShader), SHADER_COMPUTE},
		{__uuidof(ID3D11HullShader), SHADER_HULL},
		{__uuidof(ID3D11DomainShader), SHADER_DOMAIN}
	};

	const std::unordered_map<std::wstring, eShaderType> g_shader_type_map =
	{
		{L"vs", SHADER_VERTEX},
		{L"ps", SHADER_PIXEL},
		{L"gs", SHADER_GEOMETRY},
		{L"cs", SHADER_COMPUTE},
		{L"hs", SHADER_HULL},
		{L"ds", SHADER_DOMAIN}
	};

	const std::unordered_map<eShaderType, std::string> g_shader_target_map =
	{
		{SHADER_VERTEX, "vs_5_0"},
		{SHADER_PIXEL, "ps_5_0"},
		{SHADER_GEOMETRY, "gs_5_0"},
		{SHADER_COMPUTE, "cs_5_0"},
		{SHADER_HULL, "hs_5_0"},
		{SHADER_DOMAIN, "ds_5_0"}
	};

	const std::unordered_map<eShaderType, std::function<void(ID3D11Device*, ID3D11DeviceContext*, ID3D11Buffer*, UINT,
	                                                         UINT)>> g_shader_cb_bind_map =
	{
		{
			SHADER_VERTEX,
			[](ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Buffer* buffer, UINT start_slot,
			   UINT num_buffers)
			{
				context->VSSetConstantBuffers(start_slot, num_buffers, &buffer);
			}
		},
		{
			SHADER_PIXEL,
			[](ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Buffer* buffer, UINT start_slot,
			   UINT num_buffers)
			{
				context->PSSetConstantBuffers(start_slot, num_buffers, &buffer);
			}
		},
		{
			SHADER_GEOMETRY,
			[](ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Buffer* buffer, UINT start_slot,
			   UINT num_buffers)
			{
				context->GSSetConstantBuffers(start_slot, num_buffers, &buffer);
			}
		},
		{
			SHADER_COMPUTE,
			[](ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Buffer* buffer, UINT start_slot,
			   UINT num_buffers)
			{
				context->CSSetConstantBuffers(start_slot, num_buffers, &buffer);
			}
		},
		{
			SHADER_HULL,
			[](ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Buffer* buffer, UINT start_slot,
			   UINT num_buffers)
			{
				context->HSSetConstantBuffers(start_slot, num_buffers, &buffer);
			}
		},
		{
			SHADER_DOMAIN,
			[](ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Buffer* buffer, UINT start_slot,
			   UINT num_buffers)
			{
				context->DSSetConstantBuffers(start_slot, num_buffers, &buffer);
			}
		}
	};

	const std::unordered_map<eShaderType, std::function<void(ID3D11DeviceContext*, ID3D11SamplerState*, UINT, UINT)>>
	g_shader_sampler_bind_map =
	{
		{
			SHADER_VERTEX,
			[](ID3D11DeviceContext* context, ID3D11SamplerState* sampler, UINT start_slot, UINT num_samplers)
			{
				context->VSSetSamplers(start_slot, num_samplers, &sampler);
			}
		},
		{
			SHADER_PIXEL,
			[](ID3D11DeviceContext* context, ID3D11SamplerState* sampler, UINT start_slot, UINT num_samplers)
			{
				context->PSSetSamplers(start_slot, num_samplers, &sampler);
			}
		},
		{
			SHADER_GEOMETRY,
			[](ID3D11DeviceContext* context, ID3D11SamplerState* sampler, UINT start_slot, UINT num_samplers)
			{
				context->GSSetSamplers(start_slot, num_samplers, &sampler);
			}
		},
		{
			SHADER_COMPUTE,
			[](ID3D11DeviceContext* context, ID3D11SamplerState* sampler, UINT start_slot, UINT num_samplers)
			{
				context->CSSetSamplers(start_slot, num_samplers, &sampler);
			}
		},
		{
			SHADER_HULL,
			[](ID3D11DeviceContext* context, ID3D11SamplerState* sampler, UINT start_slot, UINT num_samplers)
			{
				context->HSSetSamplers(start_slot, num_samplers, &sampler);
			}
		},
		{
			SHADER_DOMAIN,
			[](ID3D11DeviceContext* context, ID3D11SamplerState* sampler, UINT start_slot, UINT num_samplers)
			{
				context->DSSetSamplers(start_slot, num_samplers, &sampler);
			}
		}
	};

	struct VPBuffer
	{
		Matrix view;
		Matrix projection;
	};

	struct TransformBuffer
	{
		Matrix scale;
		Matrix rotation;
		Matrix translation;
	};

	struct LightPositionBuffer
	{
		// due to padding, type is vector4 instead of vector3
		Vector4 position[g_max_lights];
	};

	struct LightColorBuffer
	{
		Color color[g_max_lights];
	};

	struct SpecularBuffer
	{
		float specular_power;
		float padding[3];
		Color specular_color;
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
