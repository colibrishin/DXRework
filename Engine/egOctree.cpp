#include "pch.h"
#include "egOctree.hpp"

#include "egDebugger.hpp"
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
      m_active_children_(other.m_active_children_),
      m_values_(other.m_values_),
      m_bounds_(other.m_bounds_),
      m_insertion_queue_(other.m_insertion_queue_),
      m_b_initialized_(other.m_b_initialized_),
      m_life_count_(other.m_life_count_)
    {
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
        // attempt to insert outside of the map
        if (this == nullptr)
        {
            return false;
        }

        // This will be the last node that successfully contains the object
        Octree* last = nullptr;
        std::stack<Octree*> stack;
        stack.push(this);

        // exact match (which is the smallest node that can contain the object)
        bool found = false;

        const auto bounding_value = bounding_getter::value(*obj.lock());

        while (!stack.empty())
        {
            const auto node = stack.top();
            last            = node;
            stack.pop();

            const auto& node_active_flag     = node->m_b_initialized_;
            auto&       node_active_children = node->m_active_children_;
            const auto& node_parent          = node->m_parent_;
            auto&       node_children        = node->m_children_;
            auto&       node_value           = node->m_values_;
            const auto& node_bound           = node->m_bounds_;
            const auto& node_extent          = node->Extent();
            const auto& node_center          = node->WorldCenter();

            if (!node_active_flag)
            {
                node->UpdateInternal();
            }

            const Vector3        extent_scale = node_extent * 2.f;
            const std::bitset<3> floor_bit
                    (
                     (extent_scale.x <= smallest_scale) +
                     ((extent_scale.y <= smallest_scale) << 1) +
                     ((extent_scale.z <= smallest_scale) << 2)
                    );

            // possibly the smallest node that can contain the object
            if (floor_bit.all())
            {
                node_value.push_back(obj);
                found = true;
                break;
            }

            // Check whether the object is bounded by the node
            if (bounding_value.ContainsBy(node_bound) == DirectX::CONTAINS)
            {
                // Try to insert into children for more precise containment
                for (int i = 0; i < octant_count; ++i)
                {
                    // See if there is any child that can contain the object
                    if (node_children[i])
                    {
                        if (bounding_value.ContainsBy(node_children[i]->m_bounds_) ==
                            DirectX::CONTAINS)
                        {
                            node_active_children.set(i);
                            stack.push(node_children[i].get());
                            break;
                        }
                    }
                    else
                    {
                        const auto bound       = GetBounds(node_extent, node_center, (eOctant)i);
                        const auto bound_check = bounding_value.ContainsBy(bound);

                        if (bound_check == DirectX::ContainmentType::CONTAINS)
                        {
                            // Expand the node and check same above
                            node_children[i] = std::unique_ptr<Octree>(new Octree{bound, {obj}});
                            node_children[i]->m_parent_ = node;
                            node_children[i]->Build();
                            node_active_children.set(i);
                            stack.push(node_children[i].get());
                            break;
                        }
                    }
                }
            }
        }

        if (!found && last)
        {
            last->m_values_.push_back(obj);
            return true;
        }

        // there is not match with current object (oob)
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

        // If parent has any object that can be fit into smaller node, move it to the child node
        if (!m_values_.empty())
        {
            for (auto it = m_values_.begin(); it != m_values_.end();)
            {
                if (it->expired())
                {
                    it = m_values_.erase(it);
                    continue;
                }

                const auto obj = it->lock();
                const auto obj_bound   = bounding_getter::value(*obj);
                bool found = false;

                for (int i = 0; i < octant_count; ++i)
                {
                    if (m_children_[i])
                    {
                        if (obj_bound.ContainsBy(m_children_[i]->m_bounds_) ==
                            DirectX::ContainmentType::CONTAINS)
                        {
                            // Move the object to insertion queue, and let the child node handle it
                            // By this, dirty flag will be set, and the child node will be updated
                            m_children_[i]->m_insertion_queue_.push(obj);
                            found = true;
                            break;
                        }
                    }
                }

                if (found)
                {
                    it = m_values_.erase(it);
                }
                else
                {
                    ++it;   
                }
            }
        }

        // Update children first.
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

        if (!m_values_.empty())
        {
            // Check whether the objects are still bounded by the node
            for (auto it = m_values_.begin(); it != m_values_.end();)
            {
                if (it->expired())
                {
                    it = m_values_.erase(it);
                    continue;
                }

                const auto obj = it->lock();
                const auto obj_bound   = bounding_getter::value(*obj);
                const auto bound_check = obj_bound.ContainsBy(m_bounds_);

                if (bound_check != DirectX::ContainmentType::CONTAINS)
                {
                    auto* cursor = parent();

                    while (cursor)
                    {
                        if (cursor->Insert(obj))
                        {
                            break;
                        }

                        cursor = cursor->parent();
                    }

                    if (!cursor)
                    {
                        // panic: Even root cannot contains the object, need to rebuild the whole tree.
                        root()->m_b_panic_ = true;
                        return;
                    }
                    else
                    {
                        // Object moved to parent node, remove from current obj
                        it = m_values_.erase(it);
                        continue;
                    }
                }

                ++it;
            }
        }

        if (root() == this && m_b_panic_)
        {
            Panic();
        }

        //GetDebugger().Draw(m_bounds_, DirectX::Colors::BlanchedAlmond);
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

    void Octree::Build()
    {
        std::stack<Octree*> stack;
        stack.push(this);

        while (!stack.empty())
        {
            const auto node = stack.top();
            stack.pop();

            auto& node_value = node->m_values_;
            auto& node_children = node->m_children_;
            auto& node_active_children = node->m_active_children_;
            auto& node_active_flag = node->m_b_initialized_;
            const auto node_extent = node->Extent();
            const auto node_center = node->WorldCenter();
            const auto extent_scale = node->Extent() * 2.f;

            node_active_flag = true;

            if (node_value.size() <= 1 && root() != this) continue; // Leaf node

            // Node is bigger than the max map size
            if (extent_scale.x < (float)g_max_map_size &&
                extent_scale.y < (float)g_max_map_size &&
                extent_scale.z < (float)g_max_map_size)
            {
                continue;
            }

            // Split the node into 8 octants, if it is needed, and distribute the objects
            const std::vector<BoundingBox> octants = GetBounds(node_extent, node_center);
            std::array<std::vector<WeakT>, octant_count> octant_values;

            for (int i = 0; i < octant_count; ++i)
            {
                for (auto it = node_value.begin(); it != node_value.end();)
                {
                    if (bounding_getter::value(*it->lock()).ContainsBy(octants[i]) == DirectX::CONTAINS)
                    {
                        octant_values[i].push_back(*it);
                        it = node_value.erase(it);
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
                    node_children[i]            = std::unique_ptr<Octree>(new Octree{octants[i], octant_values[i]});
                    node_children[i]->m_parent_ = node;
                    node_children[i]->Build();
                    node_active_children.set(i);
                    stack.push(node_children[i].get());
                }
            }
        }
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
        const Vector3& extent, const Vector3& center)
    {
        BoundingBox tlfb, trfb, blfb, brfb, tlbb, trbb, blbb, brbb;

        BoundingBox::CreateFromPoints(tlfb, center + (extent * tlf), center);
        BoundingBox::CreateFromPoints(trfb, center + (extent * trf), center);
        BoundingBox::CreateFromPoints(blfb, center + (extent * blf), center);
        BoundingBox::CreateFromPoints(brfb, center + (extent * brf), center);

        BoundingBox::CreateFromPoints(tlbb, center, center + (extent * tlb));
        BoundingBox::CreateFromPoints(trbb, center, center + (extent * trb));
        BoundingBox::CreateFromPoints(blbb, center, center + (extent * blb));
        BoundingBox::CreateFromPoints(brbb, center, center + (extent * brb));

        return {tlfb, trfb, blfb, brfb, tlbb, trbb, blbb, brbb};
    }

    BoundingBox __vectorcall Octree::GetBounds(
        const Vector3& extent, const Vector3& center, const eOctant region)
    {
        BoundingBox bound;

        switch (region)
        {
        case eOctant::TopLeftFront: BoundingBox::CreateFromPoints(bound, center + (extent * tlf), center);
            break;
        case eOctant::TopRightFront: BoundingBox::CreateFromPoints(bound, center + (extent * trf), center);
            break;
        case eOctant::BottomLeftFront: BoundingBox::CreateFromPoints(bound, center + (extent * blf), center);
            break;
        case eOctant::BottomRightFront: BoundingBox::CreateFromPoints(bound, center + (extent * brf), center);
            break;
        case eOctant::TopLeftBack: BoundingBox::CreateFromPoints(bound, center, center + (extent * tlb));
            break;
        case eOctant::TopRightBack: BoundingBox::CreateFromPoints(bound, center, center + (extent * trb));
            break;
        case eOctant::BottomLeftBack: BoundingBox::CreateFromPoints(bound, center, center + (extent * blb));
            break;
        case eOctant::BottomRightBack: BoundingBox::CreateFromPoints(bound, center, center + (extent * brb));
            break;
        default: throw std::logic_error("Unknown octant value given");
        }

        return bound;
    }

    Octree* Octree::root()
    {
        return m_parent_ ? m_parent_->root() : this;
    }

    Octree* Octree::parent() const
    {
        return m_parent_ ? m_parent_ : nullptr;
    }

    bool Octree::ready() const
    {
        return m_b_initialized_ && !dirty();
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
