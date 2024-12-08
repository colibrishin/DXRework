#pragma once
#include "egDXAnimCommon.hpp"
#include "egMacro.h"

namespace Engine::Graphics
{
	template <typename T>
	struct OffsetT
	{
		T     value;
		float ___p[(16 / sizeof(T)) - 1]{};

		OffsetT()
			: value(),
			  ___p{}
		{
			static_assert(sizeof(T) <= 16, "OffsetT: sizeof(T) > 16");
		}

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

		OffsetT<int> tex[g_param_buffer_slot_size];
		OffsetT<int> texArr[g_param_buffer_slot_size];
		OffsetT<int> texCube[g_param_buffer_slot_size];
		OffsetT<int> bone;
		OffsetT<int> atlas;
	};

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

BOOST_CLASS_EXPORT_KEY(Engine::Graphics::MaterialBindFlag)

BOOST_CLASS_EXPORT_KEY(Engine::Graphics::VertexElement)
