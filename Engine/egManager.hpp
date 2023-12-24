#pragma once
#include "egEntity.hpp"
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
            if (s_instance_ == nullptr)
            {
                s_instance_ = std::unique_ptr<T>(new T(SINGLETON_LOCK_TOKEN{}));
            }

            return *s_instance_;
        }

        /**
         * \brief Free the singleton instance manually.
         */
        static void Destroy()
        {
            if (s_instance_)
            {
                s_instance_.reset();
            }
        }

        virtual void Initialize(InitArgs... args) = 0;

    protected:
        Singleton();

    protected:
        struct SINGLETON_LOCK_TOKEN final {};

        inline static std::unique_ptr<T> s_instance_ = nullptr;
    };

    template <typename T, typename... InitArgs>
    Singleton<T, InitArgs...>::Singleton()
    {
        // Add singleton destroyer when the program exits.
        // however, this can be removed due to usage of unique_ptr but added for the
        // note.
        // std::atexit(&Destroy);
    }
} // namespace Engine::Abstract
