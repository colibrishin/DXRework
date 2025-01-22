#pragma once
#include <memory>
#include "RenderType.h"
#include "Source/Runtime/Core/GraphicInterface.h"
#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

namespace Engine
{
	struct RenderInstanceTask;
}

POLYMORPHIC_TYPE_MAP(Engine::RenderInstanceTask, void)

namespace Engine 
{
	struct ENGINE_RENDERPIPELINE_API RenderInstanceTask
	{
		INLINE_COMPILE_TIME_TYPENAME(RenderInstanceTask)

		virtual      ~RenderInstanceTask() = default;
		virtual void Run(Scene const* scene, RenderMap* render_map, const size_t map_size) = 0;
		virtual void Cleanup(RenderMap* render_map, const size_t map_size) = 0;
	};

	struct RenderPassTask;
	using ContextSetupFunction = std::function<void(const GraphicInterfaceContextPrimitive*)>;
}

POLYMORPHIC_TYPE_MAP(Engine::RenderPassTask, void)

namespace Engine
{
	struct ENGINE_RENDERPIPELINE_API RenderPassTask
	{
		INLINE_COMPILE_TIME_TYPENAME(RenderPassTask)

		virtual      ~RenderPassTask() = default;
		virtual void Run(
			float                                                             dt,
			bool                                                              shader_bypass,
			RenderMap const*                                                  domain_map,
			const aligned_vector<const StructuredBufferDecorator*>&           additional_sbs,
			const Graphics::SBs::LocalParamSB&                                local_param,
			const ObjectPredication&                                          predicate,
			const ContextSetupFunction&                                       prerender_predicate,
			const ContextSetupFunction&                                       postrender_predicate,
			const std::unordered_map<std::string_view, ContextSetupFunction>& prerender_predicates,
			const std::unordered_map<std::string_view, ContextSetupFunction>& postrender_predicates
		) = 0;

		virtual void Cleanup() = 0;
	};
}