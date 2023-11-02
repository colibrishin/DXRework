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
			static T instance{SINGLETON_LOCK_TOKEN{}};

		    return instance;
		}

		virtual void Initialize(InitArgs... args) = 0;
		virtual void PreUpdate() = 0;
		virtual void Update() = 0;
		virtual void PreRender() = 0;
		virtual void Render() = 0;
		virtual void FixedUpdate() = 0;

	protected:
		Singleton() = default;
		struct SINGLETON_LOCK_TOKEN final {};

	};
}
