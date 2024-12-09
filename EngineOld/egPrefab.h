#pragma once
#include "egCommon.hpp"
#include "egResource.h"

namespace Engine::Resources
{
	class Prefab : public Abstract::Resource
	{
	public:
		RESOURCE_T(RES_T_PREFAB)

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void PostUpdate(const float& dt) override;
		void FixedUpdate(const float& dt) override;

		void OnDeserialized() override;
		void OnSerialized() override;
		void OnImGui() override;

		// Add prefab object to the scene.
		void ExtractObject(const StrongScene& scene, eLayerType layer) const;

		RESOURCE_SELF_INFER_GETTER(Prefab)

		static StrongPrefab Create(
			const std::string& name, const std::filesystem::path& path, const StrongObjectBase& object
		)
		{
			const auto& pcheck = GetResourceManager().GetResourceByMetadataPath<Prefab>(path).lock();
			const auto& ncheck = GetResourceManager().GetResource<Prefab>(name).lock();

			if (pcheck || ncheck)
			{
				return ncheck ? ncheck : pcheck;
			}

			// Create new prefab object, note that constructor is private.
			const auto obj = boost::shared_ptr<Prefab>(new Prefab(object));
			GetResourceManager().AddResource(name, obj);
			return obj;
		}

	protected:
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		explicit Prefab(const StrongObjectBase& object);

		SERIALIZE_DECL
		Prefab();

		StrongObjectBase          m_object_;
		std::vector<StrongPrefab> m_children_;
	};
} // namespace Engine::Resources

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Prefab)
