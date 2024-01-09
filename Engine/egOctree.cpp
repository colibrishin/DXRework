#include "pch.h"
#include "egOctree.hpp"

#include "egTransform.h"

namespace Engine
{
    Octree::Octree()
    : m_parent_(nullptr),
      m_bounds_(Vector3::Zero, map_size_vec), // Root node is the whole map
      m_b_initialized_(false),
      m_life_count_(node_lifespan) {}

    // Deep copying
    Octree::Octree(const Octree& other)
    : m_parent_(other.m_parent_),
      m_b_initialized_(other.m_b_initialized_),
      m_life_count_(other.m_life_count_)
    {
        m_bounds_ = other.m_bounds_;
        m_values_ = other.m_values_;

        for (int i = 0; i < octant_count; ++i)
        {
            if (other.m_children_[i])
            {
                m_children_[i]            = std::unique_ptr<Octree>(other.m_children_[i].get());
                m_children_[i]->m_parent_ = this;
            }
        }
    }

    bool Octree::Insert(const WeakT& obj)
    {
        const Vector3 extent_scale = Extent() * 2.f;

        // possibly the smallest node that can contain the object
        if (extent_scale.x <= (float)smallest_scale && extent_scale.y <= (float)smallest_scale && extent_scale.z <= (float)smallest_scale)
        {
            m_values_.push_back(obj);
            return true;
        }

        // This node is not large enough to contain the object, push back to parent
        if (bounding_getter::value(*obj.lock()).ContainsBy(m_bounds_) != DirectX::CONTAINS)
        {
            if (m_parent_)
            {
                return m_parent_->Insert(obj);
            }
            else
            {
                return false;
            }
        }
        else
        {
            // Try to insert into children for more precise bounds
            const Vector3 extent = Extent();
            const Vector3 center = WorldCenter();
            bool          flag   = false;

            for (int i = 0; i < octant_count; ++i)
            {
                if (m_children_[i] && bounding_getter::value(*obj.lock()).ContainsBy(m_children_[i]->m_bounds_) ==
                    DirectX::CONTAINS)
                {
                    m_active_children_.set(i);
                    flag = m_children_[i]->Insert(obj);
                }
                else
                {
                    if (const auto bound = GetBounds(extent * 0.5f, center, (eOctant)i);
                        bounding_getter::value(*obj.lock()).ContainsBy(bound) == DirectX::CONTAINS)
                    {
                        m_children_[i]            = std::unique_ptr<Octree>(new Octree{bound});
                        m_children_[i]->m_parent_ = this;
                        m_active_children_.set(i);
                        flag = m_children_[i]->Insert(obj);
                    }
                }

                if (flag)
                {
                    break;
                }
            }

            // Unable to insert into children, insert into current node
            if (!flag)
            {
                m_values_.push_back(obj);
            }

            return true;
        }

        return false;
    }

    void Octree::Update()
    {
        unsigned attempt = 0;

        // todo: check whether the node is orphan.

        if (!m_b_initialized_)
        {
            UpdateInternal();
        }

        while (!ready())
        {
            if (attempt > retry_exhaust)
            {
                throw std::logic_error("Octree update failed.");
            }

            UpdateInternal();
            ++attempt;
        }

        // Lifecycle management
        if (empty())
        {
            m_life_count_--;
        }
        else
        {
            m_life_count_ = node_lifespan;
        }

        for (int i = 0; i < octant_count; ++i)
        {
            if (m_children_[i])
            {
                // Update children
                m_children_[i]->Update();

                // Prune empty children
                if (m_children_[i]->garbage())
                {
                    m_children_[i].reset();
                    m_active_children_.reset(i);
                }
            }
        }

        // Check whether the objects are still bounded by the node
        for (auto it = m_values_.begin(); it != m_values_.end();)
        {
            if (auto obj = it->lock())
            {
                if (bounding_getter::value(*obj).ContainsBy(m_bounds_) != DirectX::CONTAINS)
                {
                    if (!parent()->Insert(obj))
                    {
                        // panic: parent cannot update the object, need to rebuild the whole tree.
                        root()->Panic();
                        return;
                    }
                    else
                    {
                        // Object moved to parent node, remove from current node
                        m_values_.erase(it);
                    }
                }
                else
                {
                    ++it;
                }
            }
            else
            {
                // Object expired, remove from current node
                m_values_.erase(it);
            }
        }
    }

    Octree::Octree(const BoundingBox& bounds)
    : m_parent_(nullptr),
      m_b_initialized_(false),
      m_life_count_(node_lifespan)
    {
        m_bounds_ = bounds;
    }

    Octree::Octree(const BoundingBox& bounds, const std::vector<WeakT>& values)
    : m_parent_(nullptr),
      m_b_initialized_(false),
      m_life_count_(node_lifespan)
    {
        m_bounds_ = bounds;
        m_values_ = values;
    }

    void Octree::SetEnclosingCube()
    {
        const Vector3 offset = m_bounds_.Extents;
        const Vector3 new_max = m_bounds_.Center + offset;

        const float dim = MaxElement(new_max);

        // find the nearest power of 2
        const auto near_max = static_cast<float>(
            std::bit_ceil(
                static_cast<unsigned>(std::abs(dim))));

        m_bounds_.Extents = Vector3{near_max, near_max, near_max};
    }

    void Octree::Build()
    {
        if (m_values_.size() <= 1 && root() != this) return; // Leaf node

        if (DirectX::XMVector3Equal(DirectX::XMLoadFloat3(&m_bounds_.Extents), DirectX::XMVectorZero()))
        {
            SetEnclosingCube();
        }

        const auto extent_scale = Extent() * 2.f;

        if (extent_scale.x < (float)g_max_map_size && 
            extent_scale.y < (float)g_max_map_size && 
            extent_scale.z < (float)g_max_map_size)
        {
            return;
        }

        const std::vector<BoundingBox> octants = GetBounds(Extent(), WorldCenter());

        std::array<std::vector<WeakT>, octant_count> octant_values;

        for (int i = 0; i < octant_count; ++i)
        {
            for (auto it = m_values_.begin(); it != m_values_.end();)
            {
                if (bounding_getter::value(*it->lock()).ContainsBy(octants[i]) == DirectX::CONTAINS)
                {
                    octant_values[i].push_back(*it);
                    it = m_values_.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        for (int i = 0; i < octant_count; ++i)
        {
            if (!octant_values[i].empty())
            {
                m_children_[i]            = std::unique_ptr<Octree>(new Octree{octants[i], octant_values[i]});
                m_children_[i]->m_parent_ = this;
                m_active_children_.set(i);
                m_children_[i]->Build();
            }
        }

        m_b_initialized_ = true;
    }

    void Octree::Panic()
    {
        if (root() != this)
        {
            throw std::logic_error("Cannot force update from child node.");
        }

        // Keep in mind that this is rebuilding the whole tree from root node.
        m_b_initialized_ = false;

        std::stack<Octree*> stack;
        stack.push(root());

        // Scan the whole tree and collect all objects
        while (!stack.empty())
        {
            const auto node = stack.top();
            stack.pop();

            for (const auto& obj : node->m_values_)
            {
                m_insertion_queue_.push(obj);
            }

            for (int i = 0; i < octant_count; ++i)
            {
                if (node->m_children_[i])
                {
                    stack.push(node->m_children_[i].get());
                }
            }
        }

        // Rebuild the tree
        UpdateInternal();
    }

    void Octree::UpdateInternal()
    {
        // Initialize the newly created tree, or rebuild the whole tree
        if (!m_b_initialized_)
        {
            while (!m_insertion_queue_.empty())
            {
                if (auto obj = m_insertion_queue_.front().lock())
                {
                    m_values_.push_back(obj);
                }

                m_insertion_queue_.pop();
            }

            Build();
        }
        else
        {
            // Resolve the insertion queue and clear dirty flag
            while (!m_insertion_queue_.empty())
            {
                if (auto obj = m_insertion_queue_.front().lock())
                {
                    m_insertion_queue_.pop();
                    Insert(obj);
                }
            }
        }
    }

    std::vector<BoundingBox> __vectorcall Octree::GetBounds(
        const DirectX::XMFLOAT3& extent, const DirectX::XMFLOAT3& center)
    {
        return {
            BoundingBox(center + (extent * tlf), extent),
            BoundingBox(center + (extent * trf), extent),
            BoundingBox(center + (extent * blf), extent),
            BoundingBox(center + (extent * brf), extent),

            BoundingBox(center + (extent * tlb), extent),
            BoundingBox(center + (extent * trb), extent),
            BoundingBox(center + (extent * blb), extent),
            BoundingBox(center + (extent * brb), extent)
        };
    }

    BoundingBox __vectorcall Octree::GetBounds(
        const DirectX::XMFLOAT3& extent, const DirectX::XMFLOAT3& center, const eOctant region)
    {
        switch (region)
        {
        case eOctant::TopLeftFront: return {center + (extent * tlf), extent};
        case eOctant::TopRightFront: return {center + (extent * trf), extent};
        case eOctant::BottomLeftFront: return {center + (extent * blf), extent};
        case eOctant::BottomRightFront: return {center + (extent * brf), extent};
        case eOctant::TopLeftBack: return {center + (extent * tlb), extent};
        case eOctant::TopRightBack: return {center + (extent * trb), extent};
        case eOctant::BottomLeftBack: return {center + (extent * blb), extent};
        case eOctant::BottomRightBack: return {center + (extent * brb), extent};
        default: throw std::logic_error("Unknown octant value given");
        }
    }

    Octree* Octree::root()
    {
        return m_parent_ ? m_parent_->root() : this;
    }

    Octree* Octree::parent() const
    {
        return m_parent_ ? m_parent_ : nullptr;
    }

    bool Octree::ready()
    {
        return root()->m_b_initialized_ && !root()->dirty();
    }

    bool Octree::dirty() const
    {
        return !m_insertion_queue_.empty();
    }

    bool Octree::garbage() const
    {
        return m_life_count_ < 0;
    }

    bool Octree::empty() const
    {
        return m_values_.empty() && m_active_children_.none();
    }

    Vector3 Octree::MaxWorldPoint() const
    {
        return Vector3(m_bounds_.Center) + m_bounds_.Extents;
    }

    Vector3 Octree::MinWorldPoint() const
    {
        return Vector3(m_bounds_.Center) - m_bounds_.Extents;
    }

    Vector3 Octree::Extent() const
    {
        return m_bounds_.Extents;
    }

    Vector3 Octree::WorldCenter() const
    {
        return m_bounds_.Center;
    }
}
