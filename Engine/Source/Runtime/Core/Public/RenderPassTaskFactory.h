#pragma once
#include <memory>

#include "IGraphicAPI.h"
#include "TypeLibrary.h"
#include "RenderPassTask.h"

namespace Engine
{
    struct ENGINE_CORE_API IRenderPassTaskFactory
    {
        virtual ~IRenderPassTaskFactory() = default;
        virtual RenderPassTask* New() = 0;
        virtual void Release( RenderPassTask* task ) = 0;
        [[nodiscard]] virtual HashType GetTaskType() const = 0;
    };

    template <typename T> requires std::is_base_of_v<RenderPassTask, T>
    struct RenderPassTaskFactory : IRenderPassTaskFactory
    {
        RenderPassTaskFactory() {}

        ~RenderPassTaskFactory() override
        {
            for (auto* ptr : m_values_)
            {
                m_allocators_.destroy( ptr );
                m_allocators_.deallocate( ptr, 1 );
            }

            m_values_.clear();
        }

        RenderPassTask* New() override
        {
            T* task = m_allocators_.allocate( 1 );
            m_allocators_.construct( task );
            m_values_.emplace_back( task );

            return task;
        }

        void Release( RenderPassTask* task ) override
        {
            if (task->GetTypeHash() == GetTaskType())
            {
                const auto cast = static_cast<T*>( task );
                m_allocators_.destroy( cast );
                m_allocators_.deallocate( cast, 1 );
                std::erase( m_values_, task );
            }
        }

        [[nodiscard]] HashType GetTaskType() const override
        {
            return T::StaticTypeHash();
        }

    private:
        u_pool_allocator_single<T> m_allocators_{};
        std::vector<T*>            m_values_{};
    };
} // namespace Engine
