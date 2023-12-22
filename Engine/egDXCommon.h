#pragma once
#include <SimpleMath.h>
#include <boost/serialization/access.hpp>

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
        SR_RENDERED,
        SR_ANIMATION
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

    struct BonePrimitive
    {
        int        idx;
        int        parent_idx;
        float       ___p[2];
        Matrix      offset;

        BonePrimitive()
        {
            idx = 0;
            parent_idx = 0;
            std::fill_n(___p, 3, 0.f);
            offset = Matrix::Identity;
        }

        BonePrimitive(BonePrimitive&& other) noexcept
        : ___p{}
        {
            idx    = other.idx;
            parent_idx = other.parent_idx;
            offset = other.offset;
        }

        BonePrimitive(const BonePrimitive& other) noexcept
        : ___p{}
        {
            idx    = other.idx;
            parent_idx = other.parent_idx;
            offset = other.offset;
        }

        BonePrimitive& operator=(const BonePrimitive& other) noexcept
        {
            idx     = other.idx;
            parent_idx  = other.parent_idx;
            ___p[0] = 0.f;
            ___p[1] = 0.f;
            ___p[2] = 0.f;
            offset  = other.offset;

            return *this;
        }
    };

    struct BoneFrameAnimation
    {
        Matrix transform;
    };

    struct BoneAnimation
    {
        std::vector<std::pair<float, Vector3>> position;
        std::vector<std::pair<float, Vector3>> scale;
        std::vector<std::pair<float, Quaternion>> rotation;

        Vector3 GetPosition(const float time) const
        {
            if (position.size() == 1)
            {
                return position[0].second;
            }

            for (int i = 0; i < position.size() - 1; ++i)
            {
                if (time < position[i + 1].first)
                {
                    const auto& p0 = position[i];
                    const auto& p1 = position[i + 1];

                    const auto t = (time - p0.first) / (p1.first - p0.first);

                    return Vector3::Lerp(p0.second, p1.second, t);
                }
            }

            return position.back().second;
        }

        Vector3 GetScale(const float time) const
        {
            if (scale.size() == 1)
            {
                return scale[0].second;
            }

            for (size_t i = 0; i < scale.size() - 1; ++i)
            {
                if (time < scale[i + 1].first)
                {
                    const auto& p0 = scale[i];
                    const auto& p1 = scale[i + 1];

                    const auto t = (time - p0.first) / (p1.first - p0.first);

                    return Vector3::Lerp(p0.second, p1.second, t);
                }
            }

            return scale.back().second;
        }

        Quaternion GetRotation(const float time) const
        {
            for (size_t i = 0; i < rotation.size() - 1; ++i)
            {
                if (time < rotation[i + 1].first)
                {
                    const auto& p0 = rotation[i];
                    const auto& p1 = rotation[i + 1];

                    const auto t = (time - p0.first) / (p1.first - p0.first);

                    return Quaternion::Slerp(p0.second, p1.second, t);
                }
            }

            return rotation.back().second;
        }

        BoneAnimation()
        {
        }

        BoneAnimation(BoneAnimation&& other) noexcept
        {
            position = std::move(other.position);
            scale    = std::move(other.scale);
            rotation = std::move(other.rotation);
        }

        BoneAnimation(const BoneAnimation& other) noexcept
        {
            position = other.position;
            scale    = other.scale;
            rotation = other.rotation;
        }

        BoneAnimation& operator=(const BoneAnimation& other) noexcept
        {
            position = other.position;
            scale    = other.scale;
            rotation = other.rotation;

            return *this;
        }
    };

    struct AnimationPrimitive
    {
        std::string                name;
        float                      duration;
        float                      ticks_per_second;
        Matrix                     global_inverse_transform;
        std::vector<BoneAnimation> bone_animations;

        size_t GetBoneCount() const noexcept
        {
            return bone_animations.size();
        }

        BoneAnimation GetBoneAnimation(const size_t idx) const noexcept
        {
            return bone_animations[idx];
        }
    };

    struct VertexBoneElement
    {
        VertexBoneElement()
        {
            bone_count = 0;
            std::fill_n(bone_indices, 4, -1);
            std::fill_n(bone_weights, 4, 0.f);
        }

        VertexBoneElement(const VertexBoneElement& other) noexcept
        {
            bone_count = other.bone_count;
            std::ranges::copy(
                              other.bone_indices,
                              std::begin(bone_indices));
            std::ranges::copy(
                              other.bone_weights,
                              std::begin(bone_weights));
        }

        VertexBoneElement(VertexBoneElement&& other) noexcept
        {
            bone_count = other.bone_count;
            std::ranges::copy(
                              other.bone_indices,
                              std::begin(bone_indices));
            std::ranges::copy(
                              other.bone_weights,
                              std::begin(bone_weights));
        }

        void Append(const int indices, const float weight)
        {
            if (bone_count >= 4)
            {
                return;
            }

            bone_indices[bone_count] = indices;
            bone_weights[bone_count] = weight;

            bone_count++;
        }

    private:
        friend class boost::serialization::access;

        template <class Archive>
        void serialize(Archive& ar, const unsigned int version)
        {
            ar & bone_indices;
            ar & bone_weights;
            ar & bone_count;
        }

        int   bone_indices[4];
        float bone_weights[4];
        UINT  bone_count;
    };

    struct VertexElement
    {
        Vector3 position;
        Vector4 color;
        Vector2 texCoord;

        Vector3 normal;
        Vector3 tangent;
        Vector3 binormal;

        VertexBoneElement bone_element;
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
