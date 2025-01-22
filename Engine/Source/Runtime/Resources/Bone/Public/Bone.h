#pragma once
#include <map>

#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"
#include "Source/Runtime/Core/Resource/Public/Resource.h"
#include "ResourceManager/Public/ResourceManager.h"
#include "BonePrimitive.h"

#include "Bone.generated.h"

namespace Engine::Resources
{
	using namespace Graphics;
	ECLASS(resource)
	class ENGINE_BONE_API Bone : public Abstracts::Resource
	{
		GENERATE_BODY
	public:
		Bone(const BonePrimitiveMap& bone_map);
		Bone(Bone&& other) noexcept = default;
		Bone(const Bone& other);
		Bone& operator=(Bone&& other) noexcept;

		void PreUpdate(const float dt) override;
		void Update(const float dt) override;
		void FixedUpdate(const float dt) override;
		void PostUpdate(const float dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		[[nodiscard]] const BonePrimitive* GetBone(UINT idx) const;
		[[nodiscard]] const BonePrimitive* GetBone(const std::string& name);
		[[nodiscard]] bool                 Contains(const std::string& name) const;
		[[nodiscard]] const BonePrimitive* GetBoneParent(UINT idx) const;
		size_t                             GetBoneCount() const;

	protected:
		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		Bone();

		EPROPERTY()
		BonePrimitiveMap            m_bone_map;
		
		std::vector<BonePrimitive*> m_bones_index_wise_;
	};
}

