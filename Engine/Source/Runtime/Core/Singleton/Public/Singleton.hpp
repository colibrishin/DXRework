#pragma once
#include <mutex>

#include "Source/Runtime/Core/Renderable/Public/Renderable.h"

namespace Engine::Abstracts
{
	template <typename T>
	class Singleton : public Abstracts::Renderable
	{
	public:
		Singleton(const Singleton&)            = delete;
		Singleton(Singleton&&)                 = delete;
		Singleton& operator=(const Singleton&) = delete;

		static constexpr T& GetInstance()
		{
			std::lock_guard l(s_mutex_);

			if (s_instance_ == nullptr || s_destroyed_)
			{
				static_assert(SingletonChecker::ctor_void, "Singleton should not have default constructor");
				s_instance_ = std::unique_ptr<T, SingletonDeleter>(new T(SINGLETON_LOCK_TOKEN{}));
				std::call_once(s_first_call_, std::atexit, &Destroy);
				s_destroyed_ = false;
			}

			return *s_instance_;
		}

		/**
		 * \brief Free the singleton instance manually.
		 */
		static void Destroy()
		{
			std::lock_guard l(s_mutex_);
			if (s_instance_ || !s_destroyed_)
			{
				s_instance_.reset();
				s_destroyed_ = true;
			}
		}

		virtual void Initialize() = 0;

		void OnSerialized() final {}
		void OnDeserialized() final {}

	protected:
		Singleton()
		{
			static_assert(SingletonChecker::base, "Singleton must be derived from Singleton<T>");
		}

		~Singleton() override
		{
			static_assert(SingletonChecker::dtor, "Singleton should not have destructor as public");
			s_destroyed_ = true;
		}

		struct CORE_API SINGLETON_LOCK_TOKEN final {};

		struct CORE_API SingletonDeleter final
		{
			void operator()(const T* ptr) const
			{
				delete ptr;
			}
		};

	private:
		struct SingletonChecker final
		{
			constexpr static bool base      = std::is_base_of_v<Singleton, T>;
			constexpr static bool ctor_void = !std::is_default_constructible_v<T>;
			constexpr static bool dtor      = !std::is_destructible_v<T>;
		};

		inline static std::unique_ptr<T, SingletonDeleter> s_instance_ = nullptr;
		inline static std::once_flag                       s_first_call_;
		inline static std::atomic<bool>                    s_destroyed_ = true;
		inline static std::mutex                           s_mutex_     = std::mutex();
	};
} // namespace Engine::Abstract
