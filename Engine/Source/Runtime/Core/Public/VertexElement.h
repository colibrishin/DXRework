#pragma once
#include "VertexBoneElement.h"
#include "TypeLibrary.h"

#include "VertexElement.generated.h"

namespace Engine::Graphics
{
	ECLASS(serialize)
	struct ENGINE_CORE_API VertexElement
	{
		GENERATE_BODY
		
		constexpr VertexElement() :
			position(0.f, 0.f, 0.f), color(0.f, 0.f, 0.f, 1.f), texCoord(0.f, 0.f), normal(0.f, 0.f, 0.f), tangent(0.f, 0.f, 0.f), binormal(0.f, 0.f, 0.f) {}

		constexpr VertexElement(const Vector3& p, const Color& col, const Vector2& tex, const Vector3& norm, const Vector3& tangent, const Vector3& binormal, const VertexBoneElement& bone)
			: position(p), color(col), texCoord(tex), normal(norm), tangent(tangent), binormal(binormal), boneElement(bone.bone_indices_, bone.bone_weights_, bone.bone_count_)
		{}

		EPROPERTY()
		Vector3 position;

		EPROPERTY()
		Color color;

		EPROPERTY()
		Vector2 texCoord;

		EPROPERTY()
		Vector3 normal;

		EPROPERTY()
		Vector3 tangent;

		EPROPERTY()
		Vector3 binormal;

		EPROPERTY()
	    VertexBoneElement boneElement;
	};
}
