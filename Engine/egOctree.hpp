#pragma once
#include <stack>
#include <bitset>
#include <vector>
#include <boost/smart_ptr/weak_ptr.hpp>
#include "egBoundingGroup.hpp"

namespace Engine
{
    // todo: generic octree
    class Octree
    {
    private:
        using WeakT = boost::weak_ptr<Abstract::Object>;
        constexpr static size_t octant_count = 8;
        constexpr static int node_lifespan = 10;
        constexpr static unsigned retry_exhaust = 100;
        constexpr static int smallest_scale = 2;
        constexpr static Vector3 map_size_vec = Vector3{(float)g_max_map_size / 2, (float)g_max_map_size / 2, (float)g_max_map_size / 2};

        constexpr static Vector3 tlf = Vector3{-1, 1, 1};
        constexpr static Vector3 trf = Vector3{1, 1, 1};
        constexpr static Vector3 blf = Vector3{-1, -1, 1};
        constexpr static Vector3 brf = Vector3{1, -1, 1};

        constexpr static Vector3 tlb = Vector3{-1, 1, -1};
        constexpr static Vector3 trb = Vector3{1, 1, -1};
        constexpr static Vector3 blb = Vector3{-1, -1, -1};
        constexpr static Vector3 brb = Vector3{1, -1, -1};

        enum class eOctant
        {
            // Front, Left, Top = 0
            // Back, Right, Bottom = 1
            TopLeftFront = 0,
            TopRightFront = 2,
            BottomLeftFront = 4,
            BottomRightFront = 6,

            TopLeftBack = 1,
            TopRightBack = 3,
            BottomLeftBack = 5,
            BottomRightBack = 7
        };


    public:
        Octree();
        Octree(const Octree& other);

        bool Insert(const WeakT& obj);
        void Update();

    private:
        explicit Octree(const BoundingBox& bounds);
        explicit Octree(const BoundingBox& bounds, const std::vector<WeakT>& values);

        void Build();
        void Panic();
        void UpdateInternal();

        static std::vector<BoundingBox> __vectorcall GetBounds(const Vector3 &extent, const Vector3 &center);
        static BoundingBox __vectorcall GetBounds(const Vector3 &extent, const Vector3 &center, const eOctant region);

        Octree* root();
        Octree* parent() const;
        bool ready();
        bool dirty() const;
        bool garbage() const;
        bool empty() const;

        Vector3 MaxWorldPoint() const;
        Vector3 MinWorldPoint() const;

        Vector3 Extent() const;
        Vector3 WorldCenter() const;

        Octree*                   m_parent_;
        std::unique_ptr<Octree>   m_children_[octant_count];
        std::bitset<octant_count> m_active_children_;
        std::vector<WeakT>        m_values_;

        BoundingBox m_bounds_;

        std::queue<WeakT> m_insertion_queue_;
        bool              m_b_initialized_;

        int m_life_count_;
    };
}