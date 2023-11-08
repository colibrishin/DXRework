#pragma once
#include "egEntity.hpp"

namespace Engine::Abstract
{
	template <typename T, typename... InitArgs>
	class Singleton
	{
	public:
		virtual ~Singleton() = default;
		Singleton(const Singleton&) = delete;
		Singleton(Singleton&&) = delete;
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
		virtual void PreUpdate() = 0;
		virtual void Update() = 0;
		virtual void PreRender() = 0;
		virtual void Render() = 0;
		virtual void FixedUpdate() = 0;

	protected:
		Singleton();

		struct SINGLETON_LOCK_TOKEN final {};
		inline static std::unique_ptr<T> s_instance_ = nullptr;

	};

	template <typename T, typename ... InitArgs>
	Singleton<T, InitArgs...>::Singleton()
	{
		// Add singleton destroyer when the program exits.
		// however, this can be removed due to usage of unique_ptr but added for the note.
		std::atexit(&Destroy);
	}
}
