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
		virtual      ~RenderInstanceTask() = default;
		virtual void Run(Scene const* scene, RenderMap* render_map, const size_t map_size) = 0;
		virtual void Cleanup(RenderMap* render_map, const size_t map_size) = 0;

		virtual std::string_view GetTypeName() const = 0;
		virtual std::string_view GetPrettyTypeName() const = 0;
		virtual HashType GetTypeHash() const = 0;
		virtual bool IsBaseOf(HashType hash) const = 0;
	};

	struct RenderPassTask;
	using ContextSetupFunction = std::function<void(const GraphicInterfaceContextPrimitive*)>;
}

POLYMORPHIC_TYPE_MAP(Engine::RenderPassTask, void)

namespace Engine
{
	struct ENGINE_RENDERPIPELINE_API RenderPassTask
	{
		virtual      ~RenderPassTask() = default;
		virtual void Run(
			float                                   dt,
			bool                                    shader_bypass,
			RenderMap const*                        domain_map,
			const Graphics::SBs::LocalParamSB&      local_param,
			const ObjectPredication&                predicate,
			const ContextSetupFunction&				prerender_predicate,
			const ContextSetupFunction&             postrender_predicate
		) = 0;

		virtual void Cleanup() = 0;

		virtual std::string_view GetTypeName() const = 0;
		virtual std::string_view GetPrettyTypeName() const = 0;
		virtual HashType GetTypeHash() const = 0;
		virtual bool IsBaseOf(HashType hash) const = 0;
	};
}