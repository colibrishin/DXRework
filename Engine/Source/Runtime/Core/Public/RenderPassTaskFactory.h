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
        IRenderPassTaskFactory()          = default;

        IRenderPassTaskFactory( const IRenderPassTaskFactory& other )            = delete;
        IRenderPassTaskFactory& operator=( const IRenderPassTaskFactory& other ) = delete;

        IRenderPassTaskFactory( IRenderPassTaskFactory&& other ) noexcept = default;
        IRenderPassTaskFactory& operator=( IRenderPassTaskFactory&& other ) noexcept = default;

        virtual RenderPassTask* New() = 0;
        virtual void Release( RenderPassTask* task ) = 0;
        [[nodiscard]] virtual HashType GetTaskType() const = 0;
    };

    template <typename T> requires std::is_base_of_v<RenderPassTask, T>
    struct RenderPassTaskFactory : public IRenderPassTaskFactory
    {
        RenderPassTaskFactory() 
        {
        }

        ~RenderPassTaskFactory() override
        {
            m_values_.clear();
        }

        RenderPassTaskFactory( RenderPassTaskFactory&& other ) noexcept            = default;
        RenderPassTaskFactory& operator=( RenderPassTaskFactory&& other ) noexcept = default;

        RenderPassTask* New() override
        {
            // todo: allocation, can it be serialized?
            return &m_values_.emplace_back();
        }

        void Release( RenderPassTask* task ) override
        {
            if (task->GetTypeHash() == GetTaskType())
            {
                const auto cast = static_cast<T*>( task );
                std::erase_if( m_values_, [ cast ]( const T& elem ) { return &elem == cast; } );
            }
        }

        [[nodiscard]] HashType GetTaskType() const override
        {
            return T::StaticTypeHash();
        }

    private:
        std::deque<T> m_values_;
    };
} // namespace Engine
