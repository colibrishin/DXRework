#pragma once
#include <SimpleMath.h>

#include "egType.h"

namespace Engine
{
    constexpr int g_max_lights          = 8;
    constexpr int g_max_shadow_cascades = 3;

    constexpr int g_max_shadow_map_size          = 512;
    constexpr int g_max_reflect_refract_map_size = 512;

    constexpr UINT g_max_frame_latency_second = 1;
    constexpr UINT g_max_frame_latency_ms     = g_max_frame_latency_second * 1000;

    inline std::atomic<float> g_fov           = DirectX::XM_PI / 4.f;
    inline std::atomic<bool>  g_full_screen   = false;
    inline std::atomic<bool>  g_vsync_enabled = true;
    inline std::atomic<UINT>  g_window_width  = 1920;
    inline std::atomic<UINT>  g_window_height = 1080;

    inline std::atomic<float> g_screen_near = 1.f;
    inline std::atomic<float> g_screen_far  = 100.0f;

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
        CB_TYPE_SHADOW,
        CB_TYPE_SHADOW_CHUNK,
        CB_TYPE_REFRACTION,
        CB_TYPE_CLIP_PLANE,
    };

    enum eShaderResource
    {
        SR_TEXTURE = 0,
        SR_NORMAL_MAP,
        SR_SHADOW_MAP,
        SR_RENDERED
    };

    enum eSampler
    {
        SAMPLER_TEXTURE = 0,
        SAMPLER_SHADOW,
    };

    struct GraphicShadowBuffer
    {
        ComPtr<ID3D11DepthStencilView>   depth_stencil_view;
        ComPtr<ID3D11ShaderResourceView> shader_resource_view;

        ~GraphicShadowBuffer()
        {
            if (depth_stencil_view)
            {
                depth_stencil_view.Reset();
            }

            if (shader_resource_view)
            {
                shader_resource_view.Reset();
            }
        }
    };

    struct GraphicRenderedBuffer
    {
        ComPtr<ID3D11ShaderResourceView> srv;

        ~GraphicRenderedBuffer()
        {
            if (srv)
            {
                srv.Reset();
            }
        }
    };

    struct CascadeShadow
    {
        Matrix  view[g_max_shadow_cascades];
        Matrix  proj[g_max_shadow_cascades];
        Vector4 end_clip_spaces[g_max_shadow_cascades];
    };

    struct Deformation
    {
        UINT    joint_index;
        float   weight;
    };

    struct VertexElement
    {
        Vector3 position;
        Vector4 color;
        Vector2 texCoord;

        Vector3 normal;
        Vector3 tangent;
        Vector3 binormal;

        Deformation deformations[4];
    };

    struct KeyFrame
    {
        std::string name;
        UINT        frame;
        UINT        start_frame;
        UINT        end_frame;
        Matrix      global_transform;
        KeyFrame*   next = nullptr;

        ~KeyFrame()
        {
            if (next != nullptr)
            {
                delete next;
            }
        }
    };

    struct Joint
    {
        std::string name;
        UINT        index;
        UINT        parent_index;
        Matrix      global_transform;
        Matrix      local_transform;
        KeyFrame*   key_frames  = nullptr;

        ~Joint()
        {
            if (key_frames != nullptr)
            {
                delete key_frames;
            }
        }
    };

    struct PerspectiveBuffer
    {
        Matrix world;
        Matrix view;
        Matrix projection;
        Matrix invView;
        Matrix invProj;
        Matrix reflectView;
    };

    struct TransformBuffer
    {
        Matrix scale;
        Matrix rotation;
        Matrix translation;
    };

    struct LightBuffer
    {
        Matrix world[g_max_lights];
        Color  color[g_max_lights];
        int    light_count;
        float  ___p[3];
    };

    struct SpecularBuffer
    {
        float specular_power;
        float ___p[3];
        Color specular_color;
    };

    struct CascadeShadowBuffer
    {
        CascadeShadow shadow;
    };

    struct CascadeShadowBufferChunk
    {
        CascadeShadow lights[g_max_lights];
    };

    struct RefractionBuffer
    {
        float translation;
        float reflect_refract_scale;
        float ___p[2];
    };

    struct ClipPlaneBuffer
    {
        Vector4 clip_plane;
    };
} // namespace Engine
