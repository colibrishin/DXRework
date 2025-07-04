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

    struct ENGINE_CORE_API IRaytracingExtension : public virtual IGraphicAPIBase
    {
        ~IRaytracingExtension() override = default;
        INLINE_COMPILE_TIME_TYPENAME( IRaytracingExtension )

        virtual bool IsRaytracingSupported() = 0;
        virtual void InitializeRaytracing()  = 0;
        virtual void ShutdownRaytracing()    = 0;

        void UseRaytracing( const bool flag )
        {
            if ( IsRaytracingSupported() )
            {
                m_b_raytracing_ = flag;
            }
        }
        [[nodiscard]] bool ShouldUseRaytracing() const noexcept
        {
            return m_b_raytracing_;
        }

        virtual Unique<IHeapBase>  GetRaytracingHeap()      = 0;
        virtual IRaytracingShader* GetNewRaytracingShader() = 0;

        virtual void* GetRaytracingNativeInterface() = 0;
        virtual void* GetRaytracingNativePipeline()  = 0;

        virtual bool BuildTopLevelAccelerationBuffer( const IGraphicContext*   context,
                                                      RenderMap const*         render_map,
                                                      size_t                   render_map_size,
                                                      AccelStructBuffer&       out_tlas_buffer,
                                                      const ObjectPredication& predication = {} ) = 0;

        virtual void DispatchRay( const IGraphicContext*                                       context,
                                  const Resources::RaytracingShader*                           shader,
                                  const StructuredBufferTypeProxy<Graphics::SBs::LightSB>&     light,
                                  const StructuredBufferTypeProxy<Graphics::SBs::InstanceSB>&  instances,
                                  const ConstantBufferTypeProxy<Graphics::CBs::PerspectiveCB>& perspective,
                                  const ConstantBufferTypeProxy<Graphics::CBs::ParamCB>&       param,
                                  const byte_stream&                                           hit_records,
                                  const AccelStructBuffer& top_level_accel_buffer ) = 0;

        virtual void CopyRaytracingToRenderTarget( const IGraphicContext* context ) = 0;

    private:
        bool m_b_raytracing_ = false;
    };

    struct ENGINE_CORE_API IGraphicAPIAccessor
    {
    public:
        template <typename T>
            requires( std::is_base_of_v<IGraphicAPIBase, T> )
        void SetGraphicInterface()
        {
            if ( !m_graphic_ )
            {
                m_graphic_ = std::make_unique<T>();
                m_graphic_->Initialize();
            }
        }

        [[nodiscard]] IGraphicAPI& GetInterface()
        {
            return *dynamic_cast<IGraphicAPI*>( m_graphic_.get() );
        }

        [[nodiscard]] IRaytracingExtension& GetRaytracingInterface()
        {
            return *dynamic_cast<IRaytracingExtension*>( m_graphic_.get() );
        }

        void Shutdown()
        {
            if ( m_graphic_ )
            {
                m_graphic_->Shutdown();
                m_graphic_.reset();
            }
        }

    private:
        Unique<IGraphicAPIBase> m_graphic_;
    };

    extern ENGINE_CORE_API IGraphicAPIAccessor g_graphic_accessor;

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

    class ENGINE_CORE_API GraphicMemoryPool
    {
    public:
        GraphicMemoryPool() : m_allocated_size_( 0 ), m_used_size_( 0 )
        { }

        virtual ~GraphicMemoryPool() = default;

        void Update( const void* src_data, size_t count, const size_t stride )
        {
            if ( count == 0 )
            {
                count = 1;
            }

            if ( m_allocated_size_ < count )
            {
                InitializeBuffer( count, stride );
                m_allocated_size_ = count;
            }

            if ( !src_data )
            {
                return;
            }

            Map( src_data, count, stride );

            m_used_size_ = count;
        }

        virtual void Map( const void* src_data, const size_t count, const size_t stride ) = 0;

        void Release()
        {
            m_resource_.reset();
        }

        template <typename T>
        [[nodiscard]] T** GetAddressOf()
        {
            return m_resource_->GetAddressOf<T>();
        }

        template <typename T>
        [[nodiscard]] T* GetResource() const
        {
            return m_resource_->GetResource<T>();
        }

        [[nodiscard]] IGraphicResource& GetPrimitive() const
        {
            return *m_resource_;
        }

    protected:
        Unique<IGraphicResource> m_resource_;

    private:
        virtual void InitializeBuffer( const size_t count, const size_t stride ) = 0;

        size_t m_allocated_size_;
        size_t m_used_size_;
    };

#if CFG_RAYTRACING
    struct ENGINE_CORE_API AccelStructBuffer
    {
        Unique<GraphicMemoryPool> instanceDescPool;
        Unique<GraphicMemoryPool> resultPool;
        Unique<GraphicMemoryPool> scratchPool;

        bool empty = true;
    };
#endif
}