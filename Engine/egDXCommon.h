#pragma once

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>

#include "egConstant.h"
#include "egDXType.h"
#include "egMacro.h"
#include "egType.h"

namespace Engine::Graphics
{
  struct primitiveVector4
  {
    __m128 v;

  private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int file_version)
    {
      ar & v.m128_f32;
    }
  };

  struct primitiveMatrix
  {
    __m256 v[2];

  private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int file_version)
    {
      //todo: test required
      ar & v[0].m256_f32;
      ar & v[1].m256_f32;
    }
  };

  struct ParamBase
  {
  public:
    constexpr ParamBase() = default;

    template <typename T>
    void SetParam(const size_t slot, const T& param)
    {
      if constexpr (std::is_same_v<T, int>) { i_param[slot] = param; }
      else if constexpr (std::is_same_v<T, UINT>) { i_param[slot] = static_cast<int>(param); }
      else if constexpr (std::is_same_v<T, float>) { f_param[slot] = param; }
      else if constexpr (std::is_same_v<T, Vector3>)
      {
        std::memcpy(&v_param[slot], &param, sizeof(Vector3));
      }
      else if constexpr (std::is_same_v<T, Vector4>)
      {
        _mm_store_ps(v_param[slot].v.m128_f32, _mm_load_ps(param.x));
      }
      else if constexpr (std::is_same_v<T, Matrix>)
      {
        const auto row0 = const_cast<float*>(&param.m[0][0]);
        const auto row2 = const_cast<float*>(&param.m[2][0]);

        _mm256_store_ps(m_param[slot].v[0].m256_f32, _mm256_load_ps(row0));
        _mm256_store_ps(m_param[slot].v[1].m256_f32, _mm256_load_ps(row2));
      }
      else { throw std::runtime_error("Invalid type"); }
    }

    template <typename T>
    T& GetParam(const size_t slot)
    {
      if constexpr (std::is_same_v<T, int>) { return i_param[slot]; }
      else if constexpr (std::is_same_v<T, UINT>) { return reinterpret_cast<UINT&>(i_param[slot]); }
      else if constexpr (std::is_same_v<T, bool>) { return reinterpret_cast<bool&>(i_param[slot]); }
      else if constexpr (std::is_same_v<T, float>) { return f_param[slot]; }
      else if constexpr (std::is_same_v<T, Vector3>)
      {
        return reinterpret_cast<Vector3&>(v_param[slot]);
      }
      else if constexpr (std::is_same_v<T, Vector4>)
      {
        return reinterpret_cast<Vector4&>(v_param[slot]);
      }
      else if constexpr (std::is_same_v<T, Matrix>)
      {
        return reinterpret_cast<Matrix&>(m_param[slot]);
      }
      else { throw std::runtime_error("Invalid type"); }
    }

    template <typename T>
    T GetParam(const size_t slot) const
    {
      if constexpr (std::is_same_v<T, int>) { return i_param[slot]; }
      else if constexpr (std::is_same_v<T, bool>) { return static_cast<bool>(i_param[slot]); }
      else if constexpr (std::is_same_v<T, UINT>) { return static_cast<UINT>(i_param[slot]); }
      else if constexpr (std::is_same_v<T, float>) { return f_param[slot]; }
      else if constexpr (std::is_same_v<T, Vector3>)
      {
        return v_param[slot];
      }
      else if constexpr (std::is_same_v<T, Vector4>)
      {
        return v_param[slot];
      }
      else if constexpr (std::is_same_v<T, Matrix>)
      {
        Matrix m;
        _mm256_store_ps(m.m[0], m_param[slot].v[0]);
        _mm256_store_ps(m.m[2], m_param[slot].v[1]);
        return m;
      }
      else { throw std::runtime_error("Invalid type"); }
    }

  private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int file_version)
    {
      ar & f_param;
      ar & i_param;
      ar & v_param;
      ar & m_param;
    }

    constexpr static size_t max_param = 8;

    float            f_param[max_param * (sizeof(Vector4) / sizeof(float))]{};
    int              i_param[max_param * (sizeof(Vector4) / sizeof(float))]{};
    primitiveVector4 v_param[max_param]{};
    primitiveMatrix  m_param[max_param]{};
  };

  static_assert(sizeof(ParamBase) % sizeof(Vector4) == 0);
  static_assert(sizeof(ParamBase) < 2048);

  namespace SBs
  {
    struct LightSB
    {
      SB_T(SB_TYPE_LIGHT)

      Matrix world;
      Color  color;
      OffsetT<int> type;
      OffsetT<float> range;
      OffsetT<float> radius;
    };

    struct LightVPSB
    {
      SB_T(SB_TYPE_LIGHT_VP)

      Matrix  view[g_max_shadow_cascades];
      Matrix  proj[g_max_shadow_cascades];
      Vector4 end_clip_spaces[g_max_shadow_cascades];
    };

    struct InstanceSB : public ParamBase
    {
      SB_T(SB_TYPE_INSTANCE)
      SB_UAV_T(SB_TYPE_UAV_INSTANCE)

    private:
      friend class boost::serialization::access;
      template <class Archive>
      void serialize(Archive& ar, const unsigned int file_version)
      {
        ar & boost::serialization::base_object<ParamBase>(*this);
      }
    };

    struct LocalParamSB : public ParamBase
    {
      SB_T(SB_TYPE_LOCAL_PARAM)
    };

    struct InstanceModelSB : public InstanceSB
    {
      InstanceModelSB()
      {
        SetFrame(0.f);
        SetAnimDuration(0);
        SetAnimIndex(0);
        SetNoAnim(false);
        SetWorld(Matrix::Identity);
      }

      void SetFrame(const float frame) { SetParam(0, frame); }

      void SetAnimDuration(const UINT duration) { SetParam(0, (int)duration); }
      void SetAnimIndex(const UINT index) { SetParam(1, (int)index); }
      void SetNoAnim(const bool no_anim) { SetParam(2, (int)no_anim); }
      void SetAtlasX(const UINT x) { SetParam(3, (int)x); }
      void SetAtlasY(const UINT y) { SetParam(4, (int)y); }
      void SetAtlasW(const UINT w) { SetParam(5, (int)w); }
      void SetAtlasH(const UINT h) { SetParam(6, (int)h); }
      void SetRepeat(const bool repeat) { SetParam(7, (int)repeat); }

      void SetWorld(const Matrix& world) { SetParam(0, world); }
    };

    struct InstanceParticleSB : public InstanceSB
    {
    public:
      InstanceParticleSB()
      {
        SetLife(0.0f);
        SetActive(true);
        SetVelocity(Vector3::One);
        SetWorld(Matrix::Identity);
      }

      void SetLife(const float life) { SetParam(0, life); }

      void SetActive(const bool active) { SetParam(0, (int)active); }

      void SetVelocity(const Vector3& velocity) { SetParam(0, velocity); }

      void SetWorld(const Matrix& world) { SetParam(0, world); }

      Matrix& GetWorld() { return GetParam<Matrix>(0); }

      bool& GetActive() { return reinterpret_cast<bool&>(GetParam<int>(0)); }
    };

    struct MaterialSB
    {
      SB_T(SB_TYPE_MATERIAL)

      friend class boost::serialization::access;

      template <class Archive>
      void serialize(Archive& ar, const unsigned int file_version)
      {
        ar & flags;
        ar & specularPower;
        ar & reflectionTranslation;
        ar & reflectionScale;
        ar & refractionScale;
        ar & overrideColor;
        ar & specularColor;
        ar & clipPlane;
      }

      MaterialBindFlag flags;

      float specularPower;
      float reflectionTranslation;
      float reflectionScale;
      float refractionScale;

      Color        overrideColor;
      Color        specularColor;
      Vector4      clipPlane;
      OffsetT<int> repeatTexture;
    };
  }


  namespace CBs
  {
    // Constant buffers
    struct PerspectiveCB
    {
      CB_T(CB_TYPE_WVP)

      Matrix world;
      Matrix view;
      Matrix projection;

      Matrix invView;
      Matrix invProj;
      Matrix invVP;

      Matrix reflectView;
    };

    struct ParamCB : public ParamBase
    {
      CB_T(CB_TYPE_PARAM)
    };

    struct ViewportCB
    {
      RT_CB_T(RAYTRACING_CB_VIEWPORT)


      Vector2 resolution;
    };

    static_assert(sizeof(ParamCB) % sizeof(Vector4) == 0);
  }} // namespace Engine

BOOST_CLASS_EXPORT_KEY(Engine::Graphics::SBs::InstanceSB)
BOOST_CLASS_EXPORT_KEY(Engine::Graphics::ParamBase)
BOOST_CLASS_EXPORT_KEY(Engine::Graphics::SBs::InstanceModelSB)
BOOST_CLASS_EXPORT_KEY(Engine::Graphics::SBs::InstanceParticleSB)
BOOST_CLASS_EXPORT_KEY(Engine::Graphics::SBs::MaterialSB)