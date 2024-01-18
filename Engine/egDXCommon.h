#pragma once

#include "egConstant.h"
#include "egDXType.h"
#include "egMacro.h"
#include "egType.h"

namespace Engine::Graphics { namespace SBs
  {
    struct BoneSB
    {
      SB_T(SB_TYPE_BONE)
      Matrix transform;
    };

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

    struct GlobalStateCB
    {
      CB_T(CB_TYPE_GLOBAL_STATE)

      OffsetT<int> light_count;
      OffsetT<int> target_shadow;
    };

    struct MaterialCB
    {
      CB_T(CB_TYPE_MATERIAL)

      friend class Serializer;
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

    constexpr auto test = sizeof(MaterialBindFlag);
  }} // namespace Engine
