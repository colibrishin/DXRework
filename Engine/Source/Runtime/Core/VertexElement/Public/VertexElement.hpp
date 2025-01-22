#pragma once
#include "VertexBoneElement.hpp"
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

namespace Engine::Graphics
{
	struct CORE_API VertexElement
	{
		constexpr VertexElement() :
			position(0.f, 0.f, 0.f), color(0.f, 0.f, 0.f, 1.f), texCoord(0.f, 0.f), normal(0.f, 0.f, 0.f), tangent(0.f, 0.f, 0.f), binormal(0.f, 0.f, 0.f) {}

		constexpr VertexElement(const Vector3& p, const Color& col, const Vector2& tex, const Vector3& norm, const Vector3& tangent, const Vector3& binormal, const VertexBoneElement& bone)
			: position(p), color(col), texCoord(tex), normal(norm), tangent(tangent), binormal(binormal), boneElement(bone.bone_indices_, bone.bone_weights_, bone.bone_count_)
		{}

		Vector3 position;
		Color color;
		Vector2 texCoord;
		Vector3 normal;
		Vector3 tangent;
		Vector3 binormal;

	    VertexBoneElement boneElement;
	};
}

