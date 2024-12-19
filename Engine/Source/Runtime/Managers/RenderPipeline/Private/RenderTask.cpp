#include "../Public/RenderTask.h"

#include <ranges>
#include <tbb/parallel_for_each.h>

#include "Source/Runtime/Resources/Material/Public/Material.h"
#include "Source/Runtime/Resources/Shape/Public/Shape.h"

namespace Engine
{
	void GenericRenderPassTask::Run(
		const float dt,
		const bool shader_bypass,
		RenderMap const* domain_map,
		const Graphics::SBs::LocalParamSB& local_param,
		const std::atomic<uint64_t>& instance_count,
		RenderPassPrerequisiteTask* const* prerequisite,
		const size_t prerequisite_count,
		const ObjectPredication& predicate)
	{
		if (domain_map->empty())
		{
			return;
		}

		tbb::concurrent_hash_map<Weak<Resources::Material>, aligned_vector<const Graphics::SBs::InstanceSB*>> final_mapping;

		for (const auto& mtr_m : *domain_map | std::views::values)
		{
			tbb::parallel_for_each
			(
				mtr_m.begin(), mtr_m.end(), [&](const CandidateTuple& tuple)
				{
					if (!predicate || (predicate && predicate(std::get<0>(tuple).lock())))
					{
						decltype(final_mapping)::accessor acc;

						if (!final_mapping.find(acc, std::get<1>(tuple)))
						{
							final_mapping.insert(acc, std::get<1>(tuple));
						}

						for (const Graphics::SBs::InstanceSB& instance : std::get<2>(tuple))
						{
							acc->second.push_back(&instance);
						}
					}
				}
			);
		}

		GraphicInterface& gi = g_graphic_interface.GetInterface();
		const GraphicInterfaceContextReturnType& context = gi.GetNewContext(0, false, L"Render Pass");
		const GraphicInterfaceContextPrimitive& primitive = context.GetPointers();
		
		primitive.commandList->SoftReset();
		m_current_cmd_ = primitive.commandList;
		
		IStructuredBufferType<Graphics::SBs::LocalParamSB>& sb          = m_local_param_pool_.get();
		StructuredBufferTypelessBase&                               sb_typeless = sb.GetTypeless();
		sb.SetData(&primitive, 1, &local_param);
		sb_typeless.TransitionToSRV(&primitive);

		for (const auto& [mtr, sbs] : final_mapping)
		{
			m_heaps_.emplace_back(gi.GetHeap());
			m_current_heap_ = m_heaps_.back().get();

			GraphicInterfaceContextPrimitive temp_context
			{
				.commandList = primitive.commandList,
				.heap = m_current_heap_
			};

			for (size_t i = 0; i < prerequisite_count; ++i)
			{
				prerequisite[i]->Run(this);
			}

			sb_typeless.CopySRVHeap(&temp_context);
			m_current_heap_->BindGraphic(primitive.commandList);

			IStructuredBufferType<Graphics::SBs::InstanceSB>& instance = m_instance_pool_.get();
			RunImpl(dt, shader_bypass, instance, mtr.lock(), &temp_context, sbs);

			for (size_t i = 0; i < prerequisite_count; ++i)
			{
				prerequisite[i]->Cleanup(this);
			}

			m_instance_pool_.advance();

		}

		sb_typeless.TransitionCommon(&primitive);
		primitive.commandList->FlagReady();

		m_local_param_pool_.advance();
	}

	void GenericRenderPassTask::Cleanup()
	{
		m_updated_material_in_current_pass_.clear();
		m_local_param_pool_.reset();
		m_instance_pool_.reset();
	}

	CommandListBase* GenericRenderPassTask::GetCurrentCommandList() const
	{
		return m_current_cmd_;
	}

	GraphicHeapBase* GenericRenderPassTask::GetCurrentHeap() const
	{
		return m_current_heap_;
	}

	void GenericRenderPassTask::RunImpl(
		const float dt,
		const bool shader_bypass,
		IStructuredBufferType<Graphics::SBs::InstanceSB>& instance_buffer,
		const Weak<Resources::Material>& material,
		const GraphicInterfaceContextPrimitive* context,
		const aligned_vector<const Graphics::SBs::InstanceSB*>& structuredbuffers)
	{
		CheckSize<UINT>(structuredbuffers.size(), L"Warning: Renderer will take a lot of amount of instance buffers!");
		instance_buffer.SetDataContainer(context, static_cast<UINT>(structuredbuffers.size()), structuredbuffers.data());
		StructuredBufferTypelessBase& typeless = instance_buffer.GetTypeless();
		typeless.TransitionToSRV(context);
		typeless.CopySRVHeap(context);

		DrawPhase(shader_bypass, structuredbuffers.size(), material, context);

		typeless.TransitionCommon(context);
	}

	void GenericRenderPassTask::DrawPhase(
		const bool shader_bypass,
		const uint64_t instance_count,
		const Weak<Resources::Material>& material,
		const GraphicInterfaceContextPrimitive* context)
	{
		if (const Strong<Resources::Material>& mat = material.lock())
		{
			if (mat->GetResource<Resources::Shape>(0).expired())
			{
				return;
			}

			const Graphics::SBs::MaterialSB& mat_sb = mat->GetMaterialSB();
			const auto& material_resources = mat->GetResources();

			context->heap->BindGraphic(context->commandList);

			GraphicInterface& gi = g_graphic_interface.GetInterface();

			if (!shader_bypass)
			{
				const Strong<Resources::Shader>& shader = mat->GetResource<Resources::Shader>(0).lock();
				gi.Bind(context, shader.get());
			}

			for (const auto& [type, resources] : material_resources)
			{
				if (type == RES_T_ATLAS_TEX)
				{
					const auto& anim = resources.front()->GetSharedPtr<Resources::Texture>();
					gi.Bind(context, anim.get(), BIND_TYPE_SRV, RESERVED_TEX_ATLAS, 0);
					continue;
				}

				if (type == RES_T_TEX) 
				{
					for (auto it = resources.begin(); it != resources.end(); ++it)
					{
						const auto& res = *it;

						// todo: distinguish tex type
						const UINT  idx = static_cast<UINT>(std::distance(resources.begin(), it));
						const auto& tex = res->GetSharedPtr<Resources::Texture>();
						gi.Bind(context, tex.get(), BIND_TYPE_SRV, BIND_SLOT_TEX, idx);
					}
				}
			}

			if (!m_updated_material_in_current_pass_.contains(reinterpret_cast<uint64_t>(mat.get()))) 
			{
				m_material_sbs_[reinterpret_cast<uint64_t>(mat.get())] = std::move(gi.GetStructuredBuffer<Graphics::SBs::MaterialSB>());
				m_material_sbs_[reinterpret_cast<uint64_t>(mat.get())]->SetData(context, 1, &mat_sb);
				m_updated_material_in_current_pass_.insert(reinterpret_cast<uint64_t>(mat.get()));
			}

			StructuredBufferTypelessBase& material_typeless = m_material_sbs_[reinterpret_cast<uint64_t>(mat.get())]->GetTypeless();
			material_typeless.TransitionToSRV(context);
			material_typeless.CopySRVHeap(context);

			if (const Strong<Resources::Shape>& shape = mat->GetResource<Resources::Shape>(0).lock())
			{
				if (const auto& anim = shape->GetAnimations().lock())
				{
					gi.Bind(context, anim.get(), BIND_TYPE_SRV, RESERVED_TEX_BONES, 0);
				}

				for (const auto& mesh : shape->GetMeshes())
				{
					gi.Draw(context, mesh.get(), instance_count);
				}

				if (const auto& anim = shape->GetAnimations().lock())
				{
					gi.Unbind(context, anim.get(), BIND_TYPE_SRV);
				}
			}

			if (material_resources.contains(RES_T_TEX))
			{
				for (const auto& tex : material_resources.at(RES_T_TEX))
				{
					const Strong<Resources::Texture>& casted = tex->GetSharedPtr<Resources::Texture>();
					gi.Unbind(context, casted.get(), BIND_TYPE_SRV);
				}
			}

			if (material_resources.contains(RES_T_ATLAS_TEX))
			{
				const Strong<Resources::Texture>& atlas = material_resources.at(RES_T_ATLAS_TEX).front()->GetSharedPtr<Resources::Texture>();
				gi.Unbind(context, atlas.get(), BIND_TYPE_SRV);
			}

			material_typeless.TransitionCommon(context);
		}
	}
}
