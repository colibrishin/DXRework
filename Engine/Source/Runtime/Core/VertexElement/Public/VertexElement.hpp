#pragma once
#include "VertexBoneElement.hpp"
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

namespace Engine::Graphics
{
	struct CORE_API VertexElement
	{
	    Vector3 position;
	    Vector4 color;
	    Vector2 texCoord;

	    Vector3 normal;
	    Vector3 tangent;
	    Vector3 binormal;

	    VertexBoneElement boneElement;
	};
}

