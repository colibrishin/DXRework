#pragma once
#include <tuple>
#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_vector.h>

#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "Source/Runtime/Core/ConcurrentTypeLibrary/Public/ConcurrentTypeLibrary.h"
#include "Source/Runtime/Core/Allocator/Public/Allocator.h"
#include "Source/Runtime/Core/Scene/Public/Scene.hpp"
#include "Source/Runtime/Core/ObjectBase/Public/ObjectBase.hpp"
#include "Source/Runtime/Core/StructuredBuffer.h"
#include "Source/Runtime/Resources/Material/Public/Material.h"

namespace Engine
{
    using RenderInstanceIndex = uint64_t;
	using CandidateTuple = std::tuple<Weak<Engine::Abstracts::ObjectBase>, Weak<Engine::Resources::Material>, aligned_vector<Engine::Graphics::SBs::InstanceSB>>;
	using RenderMapValueType = tbb::concurrent_vector<CandidateTuple, u_align_allocator<CandidateTuple>>;
    using RenderMap = tbb::concurrent_hash_map<RenderInstanceIndex, RenderMapValueType>;

	struct RENDERER_API RenderInstanceTask 
	{
		virtual void Run(const Scene const* scene, const RenderMapValueType* render_map, const size_t map_size, std::atomic<uint64_t>& instance_count) = 0;
		virtual void Cleanup(const RenderMapValueType* render_map) = 0;
	};

	struct RENDERER_API RenderPassPrequisiteTask 
	{
		virtual void Run(RenderPassTask* task_context) = 0;
		virtual void Cleanup(RenderPassTask* task_context) = 0;
	};

	struct RENDERER_API RenderPassTask
	{
		virtual void Run(
			const float dt,
			const bool shader_bypass,
			const eShaderDomain domain, 
			const RenderMap const* domain_map,
			const std::atomic<uint64_t>& instance_count, 
			RenderPassPrequisiteTask* const* prequisite, 
			const size_t prequisite_count,
			const ObjectPredication& predicate) = 0;

		virtual void Cleanup() = 0;
	};
}