#pragma once
#include <array>
#include "TypeLibrary.h"

namespace Engine
{
	struct Simplex
	{
	private:
		std::array<Vector3, 4> m_points_;
		int                    m_size_;

	public:
		Simplex()
			: m_size_(0) {}

		Simplex& operator=(std::initializer_list<Vector3> list)
		{
			m_size_ = 0;
			for (const auto& point : list)
			{
				m_points_[m_size_++] = point;
			}

			return *this;
		}

		void push_front(const Vector3& point)
		{
			m_points_ = {point, m_points_[0], m_points_[1], m_points_[2]};
			m_size_   = std::min(m_size_ + 1, 4);
		}

		const Vector3& operator[](const int index) const
		{
			return m_points_[index];
		}

		size_t size() const
		{
			return m_size_;
		}

		auto begin() const
		{
			return m_points_.begin();
		}

		auto end() const
		{
			return m_points_.end() - (4 - m_size_);
		}
	};
	
	struct GJKExtension
	{
		static bool GetPenetration(const Weak<Components::Collider>& left, const Weak<Components::Collider>& right, Vector3& normal, float& depth);
	};
}

namespace Engine::Physics
{
	namespace GJK
	{
		bool __vectorcall GJKAlgorithm(
			const Matrix&           lhs_world,
			const Matrix&           rhs_world,
			const VertexCollection& lhs_vertices,
			const VertexCollection& rhs_vertices,
			const Vector3&          dir,
			Vector3&                normal,
			float&                  penetration,
			const std::size_t       max_gjk_iteration = 64,
			const std::size_t       max_epa_iteration = 64
		);
	} // namespace GJK
}
