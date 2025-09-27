#pragma once
#include "IGraphicAPI.h"
#include "RenderType.h"

namespace Engine
{
    using ConcurrentInstanceBufferContainer =
            tbb::concurrent_vector<StructuredBufferTypeProxy<Graphics::SBs::InstanceSB>>;

    template <typename T>
    class StructuredBufferMemoryPool;

    struct ENGINE_CORE_API RaytracingInstancePair
    {
        std::array<Strong<Resources::Texture>, g_max_texture_per_material> textures;
        StructuredBufferTypeProxy<Graphics::SBs::InstanceSB>               instance;
    };

    template <typename T>
    class StructuredBufferMemoryPool
    {
    public:
        StructuredBufferMemoryPool() = default;

        StructuredBufferMemoryPool( const StructuredBufferMemoryPool& )            = delete;
        StructuredBufferMemoryPool& operator=( const StructuredBufferMemoryPool& ) = delete;

        StructuredBufferMemoryPool( StructuredBufferMemoryPool&& other ) noexcept
        {
            operator=( std::move( other ) );
        }

        StructuredBufferMemoryPool& operator=( StructuredBufferMemoryPool&& other ) noexcept
        {
            m_resource_       = std::move( other.m_resource_ );
            m_allocated_size_ = std::move( other.m_allocated_size_ );
            m_used_size_      = std::move( other.m_used_size_ );
            m_read_offset_    = std::move( other.m_read_offset_ );
            return *this;
        }

        void resize( const size_t size )
        {
            Update( nullptr, size );
        }

        [[nodiscard]] StructuredBufferTypeProxy<T>& get()
        {
            return m_resource_[ m_read_offset_ ];
        }

        void advance()
        {
            ++m_read_offset_;

            if ( m_read_offset_ >= m_allocated_size_ )
            {
                resize( m_allocated_size_ * 2.f );
            }
        }

        void reset()
        {
            m_used_size_   = 0;
            m_read_offset_ = 0;
        }

        void Update( const T* src_data, size_t count )
        {
            if ( count == 0 )
            {
                count = 1;
            }

            UpdateSizeIfNeeded( count );

            if ( !src_data )
            {
                return;
            }

            Copy( src_data, 0, count );
            m_used_size_ = count;
        }

    private:
        void UpdateSizeIfNeeded( const size_t count )
        {
            if ( m_allocated_size_ < count )
            {
                const auto& delta  = count - m_resource_.size();
                size_t      end_it = m_resource_.size();
                m_resource_.resize( count );

                IGraphicAPI&               gi = g_graphic_accessor.GetInterface();
                const IGraphicContextImpl& context =
                        gi.GetNewContext( 0, false, L"Structured Buffer Memory pool resizing" );
                const IGraphicContext& primitive = context.GetPointers();

                primitive.commandList->SoftReset();

                for ( ; end_it < count; ++end_it )
                {
                    m_resource_.at( end_it ) = gi.GetStructuredBuffer<T>();
                    m_resource_[ end_it ].SetData( &primitive, 1, nullptr );
                }

                primitive.commandList->FlagReady();

                m_allocated_size_ = count;
            }
        }

        void Copy( const T* src_data, const size_t offset, const size_t count )
        {
            if ( m_resource_.size() < count )
            {
                throw std::logic_error( "Memory pool is not allocated enough size" );
            }

            IGraphicAPI&               gi        = g_graphic_accessor.GetInterface();
            const IGraphicContextImpl& context   = gi.GetNewContext( 0, false, L"Structured Buffer Memory pool copy" );
            const IGraphicContext&     primitive = context.GetPointers();

            primitive.commandList->SoftReset();

            for ( size_t i = offset; i < count; ++i )
            {
                m_resource_[ i ].SetData( &primitive, 1, src_data );
            }

            primitive.commandList->FlagReady();
        }

        std::vector<StructuredBufferTypeProxy<T>> m_resource_{};
        size_t                                    m_allocated_size_{};
        size_t                                    m_used_size_{};
        size_t                                    m_read_offset_{};
    };
}