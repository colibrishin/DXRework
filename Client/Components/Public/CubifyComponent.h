#pragma once
#include "Component/Public/Component.h"

#include "CubifyComponent.generated.h"

EENUM()
enum ENGINE_CLIENT_API eCubeType
{
	CUBE_TYPE_NORMAL,
	CUBE_TYPE_LADDER,
	CUBE_TYPE_WATER,
};

ECLASS(component=client, serialize)
class ENGINE_CLIENT_API CubifyComponent : public Engine::Abstracts::Component
{
public:
	inline constexpr static Vector3 s_move_offsets[4] =
	{
		{1.f, 0.f, 0.f}, // Generate cubes in x axis
		{0.f, 0.f, 1.f}, // Generate cubes in z axis
		{-1.f, 0.f, 0.f},  // Generate cubes in -x axis
		{0.f, 0.f, -1.f} // Generate cubes in -z axis
	};

	GENERATE_BODY
	~CubifyComponent() override = default;

	void Initialize() override;
	void PreUpdate(const float dt) override;
	void Update(const float dt) override;
	void UpdateCubes(bool normal);
	void PostUpdate(const float dt) override;
	void FixedUpdate(const float dt) override;

	void SetCubeDimension(const Vector3& dimension);
	void SetCubeType(eCubeType type);

	[[nodiscard]] Engine::Weak<Engine::Abstracts::ObjectBase> GetDepthNearestCube(const Vector3& pos) const;
	[[nodiscard]] eCubeType      GetCubeType() const;

	static void DispatchNormalUpdate();
	static void DispatchUpdateWithoutNormal();


	void OnSerialized() override;
	void OnDeserialized() override;
	Engine::eComponentUpdatePriorities GetUpdatePriority() const override;

#if WITH_EDITOR
	void OnUIUpdate(Engine::UIContext* const parent, const float dt) override;
#endif

protected:
	void onCollisionEnter(const Engine::Weak<Engine::Components::Collider>& other);
	void onCollisionContinue(const Engine::Weak<Engine::Components::Collider>& other);
	void onCollisionExit(const Engine::Weak<Engine::Components::Collider>& other);

private:
	inline static std::vector<Engine::Weak<CubifyComponent>> s_instance = {};
	CubifyComponent(const Engine::Weak<Engine::Abstracts::ObjectBase>& owner);
	CubifyComponent();

	void updateCubesImpl(bool normal);

	EPROPERTY()
	std::vector<Engine::LocalActorID> m_cube_ids_;
	EPROPERTY()
	Vector3                   m_cube_dimension_;
	EPROPERTY()
	eCubeType m_cube_type_;
	EPROPERTY()
	int m_z_length_;
	EPROPERTY()
	int m_y_length_;
	EPROPERTY()
	int m_x_length_;
};