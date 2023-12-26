#pragma once
#include <boost/serialization/access.hpp>
#include "egType.h"

namespace Engine
{
    constexpr int g_max_lights          = 8;
    constexpr int g_max_shadow_cascades = 3;
    constexpr int g_max_bone_count      = 4;

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
        ComPtr<ID3D11Texture2D>          texture;
        ComPtr<ID3D11DepthStencilView>   depth_stencil_view;
        ComPtr<ID3D11ShaderResourceView> shader_resource_view;
    };

    struct GraphicRenderedBuffer
    {
        ComPtr<ID3D11Texture2D>          texture;
        ComPtr<ID3D11ShaderResourceView> srv;
    };

    struct CascadeShadow
    {
        Matrix  view[g_max_shadow_cascades];
        Matrix  proj[g_max_shadow_cascades];
        Vector4 end_clip_spaces[g_max_shadow_cascades];
    };

    struct BonePrimitive
    {
        BonePrimitive()
        : m_idx_(0),
          m_parent_idx_(-1) { }

        BonePrimitive(BonePrimitive&& other) noexcept
        {
            m_idx_    = other.m_idx_;
            m_parent_idx_ = other.m_parent_idx_;
            m_inv_bind_pose_ = other.m_inv_bind_pose_;
            m_transform_ = other.m_transform_;
        }

        BonePrimitive(const BonePrimitive& other) noexcept
        {
            m_idx_    = other.m_idx_;
            m_parent_idx_ = other.m_parent_idx_;
            m_inv_bind_pose_ = other.m_inv_bind_pose_;
            m_transform_ = other.m_transform_;
        }

        BonePrimitive& operator=(const BonePrimitive& other) noexcept = default;

        __forceinline void SetIndex(const int idx) noexcept
        {
            m_idx_ = idx;
        }

        __forceinline void SetParentIndex(const int idx) noexcept
        {
            m_parent_idx_ = idx;
        }

        __forceinline void SetInvBindPose(const Matrix& inv_bind_pose) noexcept
        {
            m_inv_bind_pose_ = inv_bind_pose;
        }

        __forceinline void SetTransform(const Matrix& transform) noexcept
        {
            m_transform_ = transform;
        }

        __forceinline int GetIndex() const noexcept
        {
            return m_idx_;
        }

        __forceinline int GetParentIndex() const noexcept
        {
            return m_parent_idx_;
        }

        __forceinline const Matrix& GetInvBindPose() const noexcept
        {
            return m_inv_bind_pose_;
        }

        __forceinline const Matrix& GetTransform() const noexcept
        {
            return m_transform_;
        }

    private:
        friend class boost::serialization::access;

        template <class Archive>
        void serialize(Archive& ar, const unsigned int version)
        {
            ar & m_idx_;
            ar & m_parent_idx_;
            ar & m_inv_bind_pose_;
            ar & m_transform_;
        }

        int              m_idx_;
        int              m_parent_idx_;
        Matrix           m_inv_bind_pose_;
        Matrix           m_transform_;
    };

    struct BoneAnimationPrimitive
    {
    public:
        BoneAnimationPrimitive() :
            bone_idx(0)
        {
        }

        __forceinline void SetIndex(const int idx) noexcept
        {
            bone_idx = idx;
        }

        __forceinline void AddPosition(const float time, const Vector3& position)
        {
            m_positions_.emplace_back(time, position);
        }

        __forceinline void AddScale(const float time, const Vector3& scale)
        {
            m_scales_.emplace_back(time, scale);
        }

        __forceinline void AddRotation(const float time, const Quaternion& rotation)
        {
            m_rotations_.emplace_back(time, rotation);
        }

        __forceinline int GetIndex() const noexcept
        {
            return bone_idx;
        }

        Vector3 GetPosition(const float time) const
        {
            if (m_positions_.size() == 1)
            {
                return m_positions_[0].second;
            }

            for (int i = 0; i < m_positions_.size() - 1; ++i)
            {
                if (time < m_positions_[i + 1].first)
                {
                    const auto& p0 = m_positions_[i];
                    const auto& p1 = m_positions_[i + 1];

                    const auto t = (time - p0.first) / (p1.first - p0.first);

                    return Vector3::Lerp(p0.second, p1.second, t);
                }
            }

            return m_positions_.back().second;
        }

        Vector3 GetScale(const float time) const
        {
            if (m_scales_.size() == 1)
            {
                return m_scales_[0].second;
            }

            for (size_t i = 0; i < m_scales_.size() - 1; ++i)
            {
                if (time < m_scales_[i + 1].first)
                {
                    const auto& p0 = m_scales_[i];
                    const auto& p1 = m_scales_[i + 1];

                    const auto t = (time - p0.first) / (p1.first - p0.first);

                    return Vector3::Lerp(p0.second, p1.second, t);
                }
            }

            return m_scales_.back().second;
        }

        Quaternion GetRotation(const float time) const
        {
            for (size_t i = 0; i < m_rotations_.size() - 1; ++i)
            {
                if (time < m_rotations_[i + 1].first)
                {
                    const auto& p0 = m_rotations_[i];
                    const auto& p1 = m_rotations_[i + 1];

                    const auto t    = (time - p0.first) / (p1.first - p0.first);
                    const auto lerp = Quaternion::Slerp(p0.second, p1.second, t);
                    Quaternion norm;
                    lerp.Normalize(norm);

                    return norm;
                }
            }

            return m_rotations_.back().second;
        }

    private:
        friend class boost::serialization::access;

        template <class Archive>
        void serialize(Archive& ar, const unsigned int version)
        {
            ar & bone_idx;
            ar & m_positions_;
            ar & m_scales_;
            ar & m_rotations_;
        }

        int bone_idx;
        std::vector<std::pair<float, Vector3>> m_positions_;
        std::vector<std::pair<float, Vector3>> m_scales_;
        std::vector<std::pair<float, Quaternion>> m_rotations_;

    };

    struct AnimationPrimitive
    {
    public:
        AnimationPrimitive() :
            duration(0.f),
            ticks_per_second(0.f)
        {
        }

        AnimationPrimitive(std::string name, float duration, float ticks_per_second, Matrix global_inverse_transform) :
            name(std::move(name)),
            duration(duration),
            ticks_per_second(ticks_per_second),
            global_inverse_transform(std::move(global_inverse_transform))           
        {
        }

        AnimationPrimitive(const AnimationPrimitive& other) noexcept
        {
            name                     = other.name;
            duration                 = other.duration;
            ticks_per_second         = other.ticks_per_second;
            global_inverse_transform = other.global_inverse_transform;
            bone_animations          = other.bone_animations;

            for (auto& [key, value] : bone_animations)
            {
                bone_animations_index_wise[value.GetIndex()] = &bone_animations[key];
            }
        }

        AnimationPrimitive(AnimationPrimitive&& other) noexcept
        {
            name = other.name;
            duration = other.duration;
            ticks_per_second = other.ticks_per_second;
            global_inverse_transform = other.global_inverse_transform;
            bone_animations = std::move(other.bone_animations);
            bone_animations_index_wise = std::move(other.bone_animations_index_wise);
        }

        AnimationPrimitive& operator=(const AnimationPrimitive& other) noexcept
        {
            name                     = other.name;
            duration                 = other.duration;
            ticks_per_second         = other.ticks_per_second;
            global_inverse_transform = other.global_inverse_transform;
            bone_animations          = other.bone_animations;

            for (auto& [key, value] : bone_animations)
            {
                bone_animations_index_wise[value.GetIndex()] = &bone_animations[key];
            }

            return *this;
        }

        void Add(const std::string& name, const BoneAnimationPrimitive& bone_animation)
        {
            bone_animations[name] = bone_animation;
            bone_animations_index_wise[bone_animation.GetIndex()] = &bone_animations[name];
        }

        void SetGlobalInverseTransform(const Matrix& global_inverse_transform)
        {
            this->global_inverse_transform = global_inverse_transform;
        }

        size_t GetBoneCount() const noexcept
        {
            return bone_animations.size();
        }

        float GetDuration() const noexcept
        {
            return duration;
        }

        float GetTicksPerSecond() const noexcept
        {
            return ticks_per_second;
        }

        const Matrix& GetGlobalInverseTransform() const noexcept
        {
            return global_inverse_transform;
        }

        const BoneAnimationPrimitive* GetBoneAnimation(const int idx) const
        {
            if (bone_animations_index_wise.contains(idx))
            {
                return bone_animations_index_wise.at(idx);
            }

            return nullptr;
        }

        const BoneAnimationPrimitive* GetBoneAnimation(const std::string& name) const
        {
            if (bone_animations.contains(name))
            {
                return &bone_animations.at(name);
            }

            return nullptr;
        }

        void RebuildIndexCache()
        {
            for (const auto& [key, value] : bone_animations)
            {
                bone_animations_index_wise[value.GetIndex()] = &bone_animations[key];
            }
        }

    private:
        friend class boost::serialization::access;

        template <class Archive>
        void serialize(Archive& ar, const unsigned int version)
        {
            ar & name;
            ar & duration;
            ar & ticks_per_second;
            ar & global_inverse_transform;
            ar & bone_animations;
        }

        std::string                          name;
        float                                duration;
        float                                ticks_per_second;
        Matrix                               global_inverse_transform;
        std::map<std::string, BoneAnimationPrimitive> bone_animations;
        std::map<int, BoneAnimationPrimitive*>        bone_animations_index_wise;

    };

    struct BoneTransformElement
    {
        Matrix transform;
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
            if (bone_count >= g_max_bone_count)
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

        int   bone_indices[g_max_bone_count];
        float bone_weights[g_max_bone_count];
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
        Matrix world;
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
