#pragma once
#include <filesystem>

#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "Source/Runtime/CoreEntity/Public/Entity.h"

#include "Resource.generated.h"

// Static resource getter which infers self as type
#define RESOURCE_SELF_INFER_GETTER_DECL(TYPE)                                         \
  static Engine::Weak<TYPE> Get(const std::string& name);                          \
  static Engine::Weak<TYPE> GetByMetadataPath(const std::filesystem::path& path);  \
  static Engine::Weak<TYPE> GetByRawPath(const std::filesystem::path& path);

#define RESOURCE_SELF_INFER_GETTER_IMPL(TYPE)										  \
	Engine::Weak<TYPE> TYPE::Get(const std::string& name) { return Engine::Managers::ResourceManager::GetInstance().GetResource<TYPE>(name); }										\
	Engine::Weak<TYPE> TYPE::GetByMetadataPath(const std::filesystem::path& path) { return Engine::Managers::ResourceManager::GetInstance().GetResourceByMetadataPath<TYPE>(path); } \
	Engine::Weak<TYPE> TYPE::GetByRawPath(const std::filesystem::path& path) { return Engine::Managers::ResourceManager::GetInstance().GetResourceByRawPath<TYPE>(path); }


// Creatable resource creator which infers self as type
#define RESOURCE_SELF_INFER_CREATE_DECL(TYPE)                                     \
    static Engine::Strong<TYPE> Create(const std::string& name, const std::filesystem::path& path);

#define RESOURCE_SELF_INFER_CREATE_IMPL(TYPE)									  \
    Engine::Strong<TYPE> TYPE::Create(                                                    \
    const std::string& name, const std::filesystem::path& path)                   \
    {                                                                             \
        if (const auto pcheck = Engine::Managers::ResourceManager::GetInstance(). \
                               GetResourceByRawPath<TYPE>(path).lock();           \
            const auto ncheck = Engine::Managers::ResourceManager::GetInstance(). \
                               GetResource<TYPE>(name).lock())					  \
        {																		  \
            return ncheck;														  \
        }																		  \
        const auto obj = boost::make_shared<TYPE>(path);						  \
        Engine::Managers::ResourceManager::GetInstance().AddResource(name, obj);  \
        return obj;																  \
    }

namespace Engine
{
	using ResourceType = HashType;
}

namespace Engine::Abstracts
{
	ECLASS(abstract)
	class ENGINE_CORE_API Resource : public Entity
	{
	public:
		GENERATE_BODY

		~Resource() override;

		virtual void Load() final;
		void         Unload();

		void OnDeserialized() override;

		[[nodiscard]] bool                           IsLoaded() const;
		[[nodiscard]] const std::filesystem::path& GetPath() const;

		void SetPath(const std::filesystem::path& path);

	protected:
		Resource(std::filesystem::path path);

		virtual void Load_INTERNAL() = 0;
		virtual void Unload_INTERNAL() = 0;

	private:
		Resource();
		friend class Managers::ResourceManager;

		bool                    m_bLoaded_;

		EPROPERTY()
		std::filesystem::path m_path_;
	};
} // namespace Engine::Abstract