#pragma once
#include "egEntity.hpp"

namespace Engine::Abstract
{
	template <typename T>
	class Manager
	{
	public:
		virtual ~Manager() = default;
		Manager(const Manager&) = delete;
		Manager(Manager&&) = delete;
		Manager& operator=(const Manager&) = delete;

		static T* GetInstance()
		{
		    static std::unique_ptr<T> instance;

			if (instance == nullptr)
			{
				instance = std::unique_ptr<T>(new T{SINGLETON_LOCK_TOKEN{}});
				instance->Initialize();
			}

		    return instance.get();
		}

		virtual void Initialize() = 0;
		virtual void PreUpdate() = 0;
		virtual void Update() = 0;
		virtual void PreRender() = 0;
		virtual void Render() = 0;
		virtual void FixedUpdate() = 0;

	protected:
		Manager() = default;
		struct SINGLETON_LOCK_TOKEN final {};

	};
}
