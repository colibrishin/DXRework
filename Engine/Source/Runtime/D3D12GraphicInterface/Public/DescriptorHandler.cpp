#include "DescriptorHandler.hpp"

void Engine::DescriptorHandlerBase::PreDeconstruction()
{ 
	m_dev_.Reset();
    m_root_signature_.Reset();

	for ( ComPtr<ID3D12DescriptorHeap>& heap : m_main_descriptor_heap_ )
	{
        heap.Reset();
	}

	for ( ComPtr<ID3D12DescriptorHeap>& heap : m_main_sampler_descriptor_heap_ )
    {
        heap.Reset();
    }

	m_main_descriptor_heap_.clear();
    m_main_descriptor_heap_.clear();
}
