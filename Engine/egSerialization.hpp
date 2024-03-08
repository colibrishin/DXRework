#pragma once
#include <SimpleMath.h>
#include <filesystem>
#include <fstream>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/weak_ptr.hpp>

#include "egEntity.hpp"

namespace boost::serialization
{
  // Vector2 serialization
  template <class Archive>
  void serialize(
    Archive&           ar, DirectX::SimpleMath::Vector2& x,
    const unsigned int version
  )
  {
    ar & base_object<DirectX::XMFLOAT2>(x);
  }

  // Vector3 serialization
  template <class Archive>
  void serialize(
    Archive&           ar, DirectX::SimpleMath::Vector3& x,
    const unsigned int version
  )
  {
    ar & base_object<DirectX::XMFLOAT3>(x);
  }

  // Vector4 serialization
  template <class Archive>
  void serialize(
    Archive&           ar, DirectX::SimpleMath::Vector4& x,
    const unsigned int version
  )
  {
    ar & base_object<DirectX::XMFLOAT4>(x);
  }

  // Color serialization
  template <class Archive>
  void serialize(
    Archive&           ar, DirectX::SimpleMath::Color& x,
    const unsigned int version
  )
  {
    ar & base_object<DirectX::XMFLOAT4>(x);
  }

  // Quaternion serialization
  template <class Archive>
  void serialize(
    Archive&           ar, DirectX::SimpleMath::Quaternion& x,
    const unsigned int version
  )
  {
    ar & base_object<DirectX::XMFLOAT4>(x);
  }

  // Matrix serialization
  template <class Archive>
  void serialize(
    Archive&           ar, DirectX::SimpleMath::Matrix& x,
    const unsigned int version
  )
  {
    ar & base_object<DirectX::XMFLOAT4X4>(x);
  }

  template <class Archive>
  void serialize(
    Archive&           ar, DirectX::XMFLOAT2& x,
    const unsigned int version
  )
  {
    ar & x.x;
    ar & x.y;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, DirectX::XMFLOAT3& x,
    const unsigned int version
  )
  {
    ar & x.x;
    ar & x.y;
    ar & x.z;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, DirectX::XMFLOAT4& x,
    const unsigned int version
  )
  {
    ar & x.x;
    ar & x.y;
    ar & x.z;
    ar & x.w;
  }

  template <class Archive>
  void serialize(
     Archive&           ar, DirectX::XMFLOAT4X4& x,
         const unsigned int version
   )
  {
    ar & x._11;
    ar & x._12;
    ar & x._13;
    ar & x._14;
    ar & x._21;
    ar & x._22;
    ar & x._23;
    ar & x._24;
    ar & x._31;
    ar & x._32;
    ar & x._33;
    ar & x._34;
    ar & x._41;
    ar & x._42;
    ar & x._43;
    ar & x._44;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, DirectX::BoundingBox& x,
    const unsigned int version
  )
  {
    ar & x.Center;
    ar & x.Extents;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, DirectX::BoundingOrientedBox& x,
    const unsigned int version
  )
  {
    ar & x.Center;
    ar & x.Extents;
    ar & x.Orientation;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, DirectX::BoundingSphere& x,
    const unsigned int version
  )
  {
    ar & x.Center;
    ar & x.Radius;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEX1D_RTV& x,
    const unsigned int version
  ) { ar & x.MipSlice; }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEX1D_ARRAY_RTV& x,
    const unsigned int version
  )
  {
    ar & x.MipSlice;
    ar & x.FirstArraySlice;
    ar & x.ArraySize;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEX2D_RTV& x,
    const unsigned int version
  ) { ar & x.MipSlice; }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEX2D_ARRAY_RTV& x,
    const unsigned int version
  )
  {
    ar & x.MipSlice;
    ar & x.FirstArraySlice;
    ar & x.ArraySize;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEX2DMS_RTV& x,
    const unsigned int version
  ) { ar & x.UnusedField_NothingToDefine; }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEX2DMS_ARRAY_RTV& x,
    const unsigned int version
  )
  {
    ar & x.FirstArraySlice;
    ar & x.ArraySize;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEX3D_RTV& x,
    const unsigned int version
  )
  {
    ar & x.MipSlice;
    ar & x.FirstWSlice;
    ar & x.WSize;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEX1D_DSV& x,
    const unsigned int version
  ) { ar & x.MipSlice; }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEX1D_ARRAY_DSV& x,
    const unsigned int version
  )
  {
    ar & x.MipSlice;
    ar & x.FirstArraySlice;
    ar & x.ArraySize;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEX2D_DSV& x,
    const unsigned int version
  ) { ar & x.MipSlice; }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEX2D_ARRAY_DSV& x,
    const unsigned int version
  )
  {
    ar & x.MipSlice;
    ar & x.FirstArraySlice;
    ar & x.ArraySize;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEX2DMS_DSV& x,
    const unsigned int version
  ) { ar & x.UnusedField_NothingToDefine; }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEX2DMS_ARRAY_DSV& x,
    const unsigned int version
  )
  {
    ar & x.FirstArraySlice;
    ar & x.ArraySize;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEX1D_SRV& x,
    const unsigned int version
  )
  {
    ar & x.MostDetailedMip;
    ar & x.MipLevels;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEX1D_ARRAY_SRV& x,
    const unsigned int version
  )
  {
    ar & x.MostDetailedMip;
    ar & x.MipLevels;
    ar & x.FirstArraySlice;
    ar & x.ArraySize;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEX2D_SRV& x,
    const unsigned int version
  )
  {
    ar & x.MostDetailedMip;
    ar & x.MipLevels;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEX2D_ARRAY_SRV& x,
    const unsigned int version
  )
  {
    ar & x.MostDetailedMip;
    ar & x.MipLevels;
    ar & x.FirstArraySlice;
    ar & x.ArraySize;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEX2DMS_SRV& x,
    const unsigned int version
  ) { ar & x.UnusedField_NothingToDefine; }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEX2DMS_ARRAY_SRV& x,
    const unsigned int version
  )
  {
    ar & x.FirstArraySlice;
    ar & x.ArraySize;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEX3D_SRV& x,
    const unsigned int version
  )
  {
    ar & x.MostDetailedMip;
    ar & x.MipLevels;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEXCUBE_SRV& x,
    const unsigned int version
  )
  {
    ar & x.MostDetailedMip;
    ar & x.MipLevels;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEXCUBE_ARRAY_SRV& x,
    const unsigned int version
  )
  {
    ar & x.MostDetailedMip;
    ar & x.MipLevels;
    ar & x.First2DArrayFace;
    ar & x.NumCubes;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEX1D_UAV& x,
    const unsigned int version
  ) { ar & x.MipSlice; }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEX1D_ARRAY_UAV& x,
    const unsigned int version
  )
  {
    ar & x.MipSlice;
    ar & x.FirstArraySlice;
    ar & x.ArraySize;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEX2D_UAV& x,
    const unsigned int version
  ) { ar & x.MipSlice; }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEX2D_ARRAY_UAV& x,
    const unsigned int version
  )
  {
    ar & x.MipSlice;
    ar & x.FirstArraySlice;
    ar & x.ArraySize;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_TEX3D_UAV& x,
    const unsigned int version
  )
  {
    ar & x.MipSlice;
    ar & x.FirstWSlice;
    ar & x.WSize;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_BUFFER_RTV& x,
    const unsigned int version
  )
  {
    ar & x.ElementOffset;
    ar & x.ElementWidth;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_BUFFER_SRV& x,
    const unsigned int version
  )
  {
    ar & x.FirstElement;
    ar & x.NumElements;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_BUFFER_UAV& x,
    const unsigned int version
  )
  {
    ar & x.FirstElement;
    ar & x.NumElements;
    ar & x.Flags;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_RENDER_TARGET_VIEW_DESC& x,
    const unsigned int version
  )
  {
    ar & x.Format;
    ar & x.ViewDimension;
    ar & x.Texture1D;
    ar & x.Texture1DArray;
    ar & x.Texture2D;
    ar & x.Texture2DArray;
    ar & x.Texture2DMS;
    ar & x.Texture2DMSArray;
    ar & x.Texture3D;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_DEPTH_STENCIL_VIEW_DESC& x,
    const unsigned int version
  )
  {
    ar & x.Format;
    ar & x.ViewDimension;
    ar & x.Texture1D;
    ar & x.Texture1DArray;
    ar & x.Texture2D;
    ar & x.Texture2DArray;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_UNORDERED_ACCESS_VIEW_DESC& x,
    const unsigned int version
  )
  {
    ar & x.Format;
    ar & x.ViewDimension;
    ar & x.Texture1D;
    ar & x.Texture1DArray;
    ar & x.Texture2D;
    ar & x.Texture2DArray;
    ar & x.Texture3D;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, D3D11_SHADER_RESOURCE_VIEW_DESC& x,
    const unsigned int version
  )
  {
    ar & x.Format;
    ar & x.ViewDimension;
    ar & x.Texture1D;
    ar & x.Texture1DArray;
    ar & x.Texture2D;
    ar & x.Texture2DArray;
    ar & x.Texture2DMS;
    ar & x.Texture2DMSArray;
    ar & x.Texture3D;
    ar & x.TextureCube;
    ar & x.TextureCubeArray;
  }

  template <class Archive>
  void serialize(
    Archive&           ar, DXGI_SAMPLE_DESC& x,
    const unsigned int version
  )
  {
    ar & x.Count;
    ar & x.Quality;
  }
} // namespace boost::serialization

namespace Engine
{
  class Serializer
  {
  public:
    template <typename T>
    static bool Serialize(const std::string& filename, const boost::shared_ptr<T>& object)
    {
      object->OnSerialized();
      std::string fixed_name = filename;

      constexpr const char illegal_chars[] =
      {
        '#',
        '%',
        '&',
        '{',
        '}',
        '\\',
        '<',
        '>',
        '?',
        '/',
        ' ',
        '$',
        '!',
        '\'',
        '\"',
        ':',
        '@',
        '+',
        '|',
        '`',
        '='
      };

      for (const auto& illegal : illegal_chars)
      {
        while (const auto pos = fixed_name.find(illegal))
        {
          if (pos == std::string::npos)
          {
            break;
          }
          else
          {
            fixed_name.replace(pos, 1, "_");
          }
        }
      }

      //int                   i               = 0;
      //std::string           tagged_filename = fixed_name + "_%d";
      //char                  buffer[1024]    = {};
      //std::filesystem::path final_path      = fixed_name;
      std::string extension = ".meta";

      //while (std::filesystem::exists(final_path.string() + extension))
      //{
      //  sprintf_s(buffer, 1024, tagged_filename.c_str(), i++);
      //  final_path = buffer;
      //}

      const std::filesystem::path folder = object->GetPrettyTypeName();

      if (!std::filesystem::exists(folder))
      {
        std::filesystem::create_directory(folder);
      }

      std::filesystem::path final_filename = fixed_name + extension;

      std::filesystem::path final_path =  folder / final_filename;
      object->m_meta_path_             = final_path.concat(extension);
      object->m_meta_str_              = final_path.string();

      const auto entity = boost::static_pointer_cast<Abstract::Entity>(object);

      std::fstream                    stream(final_path, std::ios::out | std::ios::binary);
      boost::archive::binary_oarchive archive(stream);
      archive << entity;
      return true;
    }

    template <typename T>
    static boost::shared_ptr<T> Deserialize(const std::string& filename)
    {
      boost::shared_ptr<Abstract::Entity> object;
      std::fstream                        stream(filename, std::ios::in  | std::ios::binary);

      if (!stream.is_open()) { throw std::runtime_error("Failed to open file for deserialization"); }

      boost::archive::binary_iarchive archive(stream);
      archive >> object;
      object->OnDeserialized();
      return boost::static_pointer_cast<T>(object);
    }
  };
} // namespace Engine
