#pragma once
#include <SimpleMath.h>
#include <filesystem>
#include <fstream>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
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

namespace boost::serialization
{
    // Vector2 serialization
    template <class Archive>
    void serialize(
        Archive&           ar, DirectX::SimpleMath::Vector2& x,
        const unsigned int version)
    {
        ar & x.x;
        ar & x.y;
    }

    // Vector3 serialization
    template <class Archive>
    void serialize(
        Archive&           ar, DirectX::SimpleMath::Vector3& x,
        const unsigned int version)
    {
        ar & x.x;
        ar & x.y;
        ar & x.z;
    }

    // Vector4 serialization
    template <class Archive>
    void serialize(
        Archive&           ar, DirectX::SimpleMath::Vector4& x,
        const unsigned int version)
    {
        ar & x.x;
        ar & x.y;
        ar & x.z;
        ar & x.w;
    }

    // Color serialization
    template <class Archive>
    void serialize(
        Archive&           ar, DirectX::SimpleMath::Color& x,
        const unsigned int version)
    {
        ar & x.x;
        ar & x.y;
        ar & x.z;
        ar & x.w;
    }

    // Quaternion serialization
    template <class Archive>
    void serialize(
        Archive&           ar, DirectX::SimpleMath::Quaternion& x,
        const unsigned int version)
    {
        ar & x.x;
        ar & x.y;
        ar & x.z;
        ar & x.w;
    }

    // Matrix serialization
    template <class Archive>
    void serialize(
        Archive&           ar, DirectX::SimpleMath::Matrix& x,
        const unsigned int version)
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
        Archive&           ar, Engine::BonePrimitive& x,
        const unsigned int version)
    {
        ar & x.idx;
        ar & x.name;
        ar & x.offset;
    }

    template <class Archive>
    void serialize(
        Archive&           ar, Engine::VertexElement& x,
        const unsigned int version)
    {
        ar & x.position;
        ar & x.texCoord;
        ar & x.normal;
        ar & x.tangent;
        ar & x.binormal;
        ar & x.color;
        ar & x.bone_element;
    }

    template <class Archive>
    void serialize(
        Archive&           ar, Engine::KeyFrame& x,
        const unsigned int version)
    {
        ar & x.frame;
        ar & x.scale;
        ar & x.rotation;
        ar & x.translation;
    }

    template <class Archive>
    void serialize(
        Archive&           ar, Engine::AnimationPrimitive& x,
        const unsigned int version)
    {
        ar & x.keyframes;
        ar & x.name;
    }

    template <class Archive>
    void serialize(
        Archive&           ar, DirectX::XMFLOAT3& x,
        const unsigned int version)
    {
        ar & x.x;
        ar & x.y;
        ar & x.z;
    }

    template <class Archive>
    void serialize(
        Archive&           ar, DirectX::BoundingBox& x,
        const unsigned int version)
    {
        ar & x.Center;
        ar & x.Extents;
    }

    //
} // namespace boost::serialization

namespace Engine
{
    class Serializer
    {
    public:
        template <typename T>
        static void Serialize(const std::string& filename, const T& object)
        {
            // @todo: block serialization of non-serializable types (e.g., weak_ptr)
            std::fstream                  stream(filename, std::ios::out);
            boost::archive::text_oarchive archive(stream);
            archive << object;
        }

        template <typename T>
        static boost::shared_ptr<T> Deserialize(const std::string& filename)
        {
            // @todo: block deserialization of non-serializable types (e.g., weak_ptr)
            boost::shared_ptr<T>          object = boost::shared_ptr<T>();
            std::fstream                  stream(filename, std::ios::in);
            boost::archive::text_iarchive archive(stream);
            archive >> object;
            object->OnDeserialized();
            return object;
        }
    };
} // namespace Engine
