#pragma once
#include <assimp/Importer.hpp>

#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include <filesystem>

namespace Engine 
{
    struct ShapeImporter
    {
        static bool Import
        (
            const GenericString& name,
            const boost::filesystem::path& path,
            std::vector<Strong<Resources::Mesh>>& meshes,
            Strong<Resources::Shape>& shape, 
            Strong<Resources::AnimationTexture>& animstex
        );

        __forceinline static Matrix __vectorcall AiMatrixToDirectXTranspose(const aiMatrix4x4& from)
        {
            return Matrix
            (
                from.a1, from.b1, from.c1, from.d1,
                from.a2, from.b2, from.c2, from.d2,
                from.a3, from.b3, from.c3, from.d3,
                from.a4, from.b4, from.c4, from.d4
            );
        }

        static Assimp::Importer s_assimp_importer_;
    };
}