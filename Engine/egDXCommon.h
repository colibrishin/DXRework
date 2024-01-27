#pragma once

#include "egConstant.h"
#include "egDXType.h"
#include "egMacro.h"
#include "egType.h"

namespace Engine::Graphics
{
  struct ParamBase
  {
  public:
    template <typename T>
    void SetParam(const size_t slot, const T& param)
    {
      if constexpr (std::is_same_v<T, int>) { i_param[slot] = param; }
      else if constexpr (std::is_same_v<T, float>) { f_param[slot] = param; }
      else if constexpr (std::is_same_v<T, Vector3>) { v_param[slot] = Vector4(param); }
      else if constexpr (std::is_same_v<T, Vector4>) { v_param[slot] = param; }
      else if constexpr (std::is_same_v<T, Matrix>) { m_param[slot] = param; }
      else { throw std::runtime_error("Invalid type"); }
    }

    template <typename T>
    T GetParam(const size_t slot) const
    {
      if constexpr (std::is_same_v<T, int>) { return i_param[slot]; }
      else if constexpr (std::is_same_v<T, float>) { return f_param[slot]; }
      else if constexpr (std::is_same_v<T, Vector3>) { return Vector3(v_param[slot]); }
      else if constexpr (std::is_same_v<T, Vector4>) { return v_param[slot]; }
      else if constexpr (std::is_same_v<T, Matrix>) { return m_param[slot]; }
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

    float   f_param[max_param * (sizeof(Vector4) / sizeof(float))]{};
    int     i_param[max_param * (sizeof(Vector4) / sizeof(float))]{};
    Vector4 v_param[max_param]{};
    Matrix  m_param[max_param]{};
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
    };

    struct LightVPSB
    {
      SB_T(SB_TYPE_SHADOW)

      Matrix  view[g_max_shadow_cascades];
      Matrix  proj[g_max_shadow_cascades];
      Vector4 end_clip_spaces[g_max_shadow_cascades];
    };

    struct InstanceSB : public ParamBase
    {
      SB_T(SB_TYPE_INSTANCE)
      SB_UAV_T(SB_TYPE_UAV_INSTANCE)
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

      void SetWorld(const Matrix& world) { SetParam(0, world); }
    };

    struct InstanceParticleSB : public InstanceSB
    {
    public:
      InstanceParticleSB()
      {
        SetLife(0.0f);
        SetVelocity(Vector3::One);
        SetWorld(Matrix::Identity);
      }

      void SetLife(const float life) { SetParam(1, life); }

      void SetVelocity(const Vector3& velocity) { SetParam(0, velocity); }

      void SetWorld(const Matrix& world) { SetParam(0, world); }

      Matrix GetWorld() const { return GetParam<Matrix>(0); }
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

      Matrix reflectView;
    };

    struct TransformCB
    {
      CB_T(CB_TYPE_TRANSFORM)

      Matrix world;
    };

    struct MaterialCB
    {
      CB_T(CB_TYPE_MATERIAL)
      friend class boost::serialization::access;

      template <class Archive>
      void serialize(Archive& ar, const unsigned int file_version)
      {
        ar & flags;
        ar & specular_power;
        ar & reflection_translation;
        ar & reflection_scale;
        ar & refraction_scale;
        ar & override_color;
        ar & specular_color;
        ar & clip_plane;
      }

      MaterialBindFlag flags;

      float specular_power;
      float reflection_translation;
      float reflection_scale;
      float refraction_scale;

      Color   override_color;
      Color   specular_color;
      Vector4 clip_plane;
    };

    struct ParamCB : public ParamBase
    {
      CB_T(CB_TYPE_PARAM)
    };

    static_assert(sizeof(ParamCB) % sizeof(Vector4) == 0);
  }} // namespace Engine
