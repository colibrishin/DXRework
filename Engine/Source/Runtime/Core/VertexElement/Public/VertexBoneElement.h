#pragma once
#include <algorithm>
#include <ranges>
#include <vector>

#include "VertexBoneElement.generated.h"

namespace Engine::Graphics
{
	inline static constexpr std::size_t g_max_bone_count = 4;

	ECLASS(serialize)
	struct ENGINE_CORE_API VertexBoneElement
	{
		GENERATE_BODY
		
	    constexpr VertexBoneElement()
	    {
	        bone_count_ = 0;
	        std::fill_n(bone_indices_, 4, -1);
	        std::fill_n(bone_weights_, 4, 0.f);
	    }

		constexpr VertexBoneElement(const int indices[g_max_bone_count], const float weights[g_max_bone_count], const uint32_t count)
	    {
			bone_count_ = count;
	    	
		    for (size_t i = 0; i < bone_count_; ++i)
		    {
			    bone_indices_[i] = indices[i];
		    	bone_weights_[i] = weights[i];
		    }
	    }

		constexpr VertexBoneElement& operator=(const VertexBoneElement& other)
	    {
		    bone_count_ = other.bone_count_;
			std::ranges::copy
	                (
	                    other.bone_indices_,
	                    std::begin(bone_indices_)
	                );
	        std::ranges::copy
	                (
	                    other.bone_weights_,
	                    std::begin(bone_weights_)
	                );

			return *this;
	    }

	    constexpr VertexBoneElement(const VertexBoneElement& other) noexcept
	    {
	        bone_count_ = other.bone_count_;
	        std::ranges::copy
	                (
	                    other.bone_indices_,
	                    std::begin(bone_indices_)
	                );
	        std::ranges::copy
	                (
	                    other.bone_weights_,
	                    std::begin(bone_weights_)
	                );
	    }

	    void Append(const int indices, const float weight)
	    {
	        if (bone_count_ >= g_max_bone_count)
	        {
	            return;
	        }

	        bone_indices_[bone_count_] = indices;
	        bone_weights_[bone_count_] = weight;

	        bone_count_++;
	    }

	    [[nodiscard]] std::vector<uint32_t> GetIndices() const
	    {
	        return {bone_indices_, bone_indices_ + bone_count_};
	    }

	    int      bone_indices_[g_max_bone_count]{};
	    float    bone_weights_[g_max_bone_count]{};
	    uint32_t bone_count_;
	};
}