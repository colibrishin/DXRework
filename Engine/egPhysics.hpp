#pragma once
#include <SimpleMath.h>
#include <array>

#undef min

namespace Engine::Physics
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

    inline Vector3 __vectorcall GetActivePolarity(const Vector3& vel)
    {
        const auto signx =
                vel.x > 0.f ? 1.f : (std::fabsf(vel.x) == g_epsilon ? 0.f : -1.f);
        const auto signy =
                vel.y > 0.f ? 1.f : (std::fabsf(vel.y) == g_epsilon ? 0.f : -1.f);
        const auto signz =
                vel.z > 0.f ? 1.f : (std::fabsf(vel.z) == g_epsilon ? 0.f : -1.f);

        return {signx, signy, signz};
    }

    extern Vector3 __vectorcall EvalFriction(const Vector3& vel, float mu, float dt);
    extern Vector3 __vectorcall EvalDrag(const Vector3& vel, float k);
    extern Vector3 __vectorcall EvalGravity(float invMass, float dt);
} // namespace Engine::Physics
