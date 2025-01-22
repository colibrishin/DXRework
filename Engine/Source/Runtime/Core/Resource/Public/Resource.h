#pragma once
#include <filesystem>

#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "Source/Runtime/CoreEntity/Public/Entity.h"

#include "Resource.generated.h"

// Static resource getter which infers self as type
#define RESOURCE_SELF_INFER_GETTER(TYPE) \
	template <typename Void = void> requires (std::is_base_of_v<Engine::Abstracts::Resource, TYPE##>)\
	static Engine::Weak<TYPE> Get(const std::string& name) { return Engine::Managers::ResourceManager::GetInstance().GetResource<TYPE>(name); }\
	template <typename Void = void> requires (std::is_base_of_v<Engine::Abstracts::Resource, TYPE##>)\
	static Engine::Weak<TYPE> GetByMetadataPath(const std::filesystem::path& meta_path) { return Engine::Managers::ResourceManager::GetInstance().GetResourceByMetadataPath<TYPE>(meta_path); } \
	template <typename Void = void> requires (std::is_base_of_v<Engine::Abstracts::Resource, TYPE##>)\
	static Engine::Weak<TYPE> GetByRawPath(const std::filesystem::path& path) { return Engine::Managers::ResourceManager::GetInstance().GetResourceByRawPath<TYPE>(path); }

// Static resource create function
// using std::enable_if_t<...> in template throws error of undefined class, however requires keyword works fine.
#define RESOURCE_SELF_INFER_CREATE(TYPE)\
template <typename... Args> requires (std::is_base_of_v<Engine::Abstracts::Resource, TYPE##>)\
static Engine::Strong<TYPE> Create(const std::string_view name, Args&&... args)\
{\
if (!name.empty() && Engine::Managers::ResourceManager::GetInstance().GetResource<##TYPE##>(name).lock()) { return {}; }\
const auto obj = boost::make_shared<##TYPE##>(std::forward<Args>(args)...);\
Engine::Managers::ResourceManager::GetInstance().AddResource(name, obj);\
return obj;\
}

namespace Engine
{
	using ResourceType = HashType;
}

namespace Engine::Abstracts
{
	ECLASS(abstract, serialize)
	class ENGINE_CORE_API Resource : public Entity
	{
	public:
		GENERATE_BODY

		~Resource() override;

		virtual void Load() final;
		void         Unload();

		void OnDeserialized() override;

		[[nodiscard]] bool                         IsLoaded() const;
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