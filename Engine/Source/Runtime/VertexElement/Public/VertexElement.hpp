#pragma once
#include <boost/serialization/access.hpp>

#include "VertexBoneElement.hpp"
#include "Source/Runtime/TypeLibrary/Public/TypeLibrary.h"

namespace Engine::Graphics
{
	struct VertexElement
	{
	    Vector3 position;
	    Vector4 color;
	    Vector2 texCoord;

	    Vector3 normal;
	    Vector3 tangent;
	    Vector3 binormal;

	    VertexBoneElement boneElement;

	private:
	    friend class boost::serialization::access;

	    template <class Archive>
	    void serialize(Archive& ar, const unsigned int file_version)
	    {
	        ar & position;
	        ar & color;
	        ar & texCoord;
	        ar & normal;
	        ar & tangent;
	        ar & binormal;
	        ar & boneElement;
	    }
	};
}

