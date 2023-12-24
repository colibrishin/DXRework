#pragma once
#include "egEntity.hpp"
#include "egType.h"
#include "egRenderable.h"

namespace Engine::Abstract
{
    template <typename T, typename... InitArgs>
    class Singleton : public Renderable
    {
    public:
        ~Singleton() override                  = default;
        Singleton(const Singleton&)            = delete;
        Singleton(Singleton&&)                 = delete;
        Singleton& operator=(const Singleton&) = delete;

        static T& GetInstance()
        {
            std::lock_guard l(s_mutex_);

            if (s_instance_ == nullptr)
            {
                s_instance_ = std::unique_ptr<T>(new T(SINGLETON_LOCK_TOKEN{}));
                std::call_once(s_first_call_, std::atexit, &Destroy);
            }

            return *s_instance_;
        }

        /**
         * \brief Free the singleton instance manually.
         */
        static void Destroy()
        {
            std::lock_guard l(s_mutex_);
            if (s_instance_)
            {
                s_instance_.reset();
            }
        }

        virtual void Initialize(InitArgs... args) = 0;

    protected:
        Singleton();

        struct SINGLETON_LOCK_TOKEN final {};

        inline static std::unique_ptr<T> s_instance_ = nullptr;
        inline static std::once_flag s_first_call_;
        inline static std::mutex s_mutex_ = std::mutex();
    };

    template <typename T, typename... InitArgs>
    Singleton<T, InitArgs...>::Singleton()
    {
    }
} // namespace Engine::Abstract
