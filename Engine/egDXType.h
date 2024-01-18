#pragma once
#include "egDXAnimCommon.hpp"
#include "egMacro.h"

namespace Engine::Graphics { namespace DXPacked
  {
    struct ShadowVPResource
    {
      ComPtr<ID3D11Texture2D>          texture;
      ComPtr<ID3D11DepthStencilView>   depth_stencil_view;
      ComPtr<ID3D11ShaderResourceView> shader_resource_view;
    };

    struct RenderedResource
    {
      ComPtr<ID3D11Texture2D>          texture;
      ComPtr<ID3D11ShaderResourceView> srv;
    };
  }

  template <typename T>
  struct OffsetT
  {
    T     value;
    float ___p[(16 / sizeof(T)) - 1]{};

    OffsetT()
      : value(),
        ___p{} { static_assert(sizeof(T) <= 16, "OffsetT: sizeof(T) > 16"); }

    ~OffsetT() = default;

    OffsetT(const T& v)
      : value(v) {}

    OffsetT& operator=(const T& v)
    {
      value = v;
      return *this;
    }

    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int file_version)
    {
      ar & value;
      ar & ___p;
    }
  };

  struct MaterialBindFlag
  {
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int file_version)
    {
      ar & tex;
      ar & texArr;
      ar & texCube;
      ar & bone;
    }

    OffsetT<int> tex[g_max_slot_per_texture];
    OffsetT<int> texArr[g_max_slot_per_texture];
    OffsetT<int> texCube[g_max_slot_per_texture];
    OffsetT<int> bone;
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
}
