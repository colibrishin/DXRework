#pragma once
#include "egResource.h"

namespace Engine::Resources
{
	using namespace Graphics;

	class Bone : public Abstract::Resource
	{
	public:
		RESOURCE_T(RES_T_BONE)

		Bone(const BonePrimitiveMap& bone_map);
		Bone(Bone&& other) noexcept = default;
		Bone(const Bone& other);
		Bone& operator=(Bone&& other) noexcept;

		void PreUpdate(const float& dt) override;
		void Update(const float& dt) override;
		void FixedUpdate(const float& dt) override;
		void PostUpdate(const float& dt) override;

		void OnSerialized() override;
		void OnDeserialized() override;

		[[nodiscard]] const BonePrimitive* GetBone(UINT idx) const;
		[[nodiscard]] const BonePrimitive* GetBone(const std::string& name);
		[[nodiscard]] bool                 Contains(const std::string& name) const;
		[[nodiscard]] const BonePrimitive* GetBoneParent(UINT idx) const;
		size_t                             GetBoneCount() const;

		RESOURCE_SELF_INFER_GETTER(Bone)

	protected:
		SERIALIZE_DECL

		void Load_INTERNAL() override;
		void Unload_INTERNAL() override;

	private:
		Bone();

		BonePrimitiveMap            m_bone_map;
		std::vector<BonePrimitive*> m_bones_index_wise_;
	};
}

BOOST_CLASS_EXPORT_KEY(Engine::Resources::Bone)
