#pragma once
#include <array>
#include <bitset>
#include <stack>
#include <vector>
#include <boost/smart_ptr/weak_ptr.hpp>

#include "Source/Runtime/Core/TypeLibrary/Public/TypeLibrary.h"

namespace Engine
{
	inline static constexpr float g_max_map_size = 2048;

	// todo: generic octree
	template <typename WeakT, typename BoundingValueGetter, float Epsilon = 0.0001f>
	class Octree
	{
	private:
		constexpr static size_t   octant_count   = 8;
		constexpr static int      node_lifespan  = 10;
		constexpr static unsigned retry_exhaust  = 100;
		constexpr static int      smallest_scale = 2;
		constexpr static Vector3  map_size_vec   = Vector3{
			static_cast<float>(g_max_map_size) / 2, static_cast<float>(g_max_map_size) / 2,
			static_cast<float>(g_max_map_size) / 2
		};

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
			TopLeftFront     = 0,
			TopRightFront    = 2,
			BottomLeftFront  = 4,
			BottomRightFront = 6,

			TopLeftBack     = 1,
			TopRightBack    = 3,
			BottomLeftBack  = 5,
			BottomRightBack = 7
		};

	public:
		Octree(): m_parent_(nullptr),
		  m_bounds_(Vector3::Zero, map_size_vec), // Root node is the whole map
		  m_b_initialized_(false),
		  m_life_count_(node_lifespan) {}

		Octree(const Octree& other)
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
					m_children_[i]            = std::make_unique<Octree>(*other.m_children_[i]);
					m_children_[i]->m_parent_ = this;
				}
			}
		}

		const std::vector<WeakT>& Read() const
		{
			return m_values_;
		}

		std::array<const Octree*, 8> Next() const
		{
			std::array<const Octree*, 8> next;

			for (int i = 0; i < octant_count; ++i)
			{
				if (m_children_[i])
				{
					next[i] = m_children_[i].get();
				}
				else
				{
					next[i] = nullptr;
				}
			}

			return next;
		}

		bool Contains(const Vector3& point) const
		{
			return m_bounds_.Contains(point) == DirectX::ContainmentType::CONTAINS;
		}

		// Checks if the given point or bounds intersects with the node.
		template <typename T>
		bool Intersects(const T& point_or_bounds) const
		{
			return m_bounds_.Intersects(point_or_bounds);
		}

		// Check if the given ray intersects with the node.
		bool Intersects(const Vector3& point, const Vector3& dir, float& distance) const
		{
			return m_bounds_.Intersects(point, dir, distance);
		}

		template <typename T>
		bool Contains(const T& point_or_bounds) const
		{
			return m_bounds_.Contains(point_or_bounds) == DirectX::ContainmentType::CONTAINS;
		}

		// Gets the distance between node bounding box and the given point.
		float Distance(const Vector3& point) const
		{
			return std::sqrtf(Vector3::DistanceSquared(m_bounds_.Center, point));
		}

		UINT ActiveChildren() const
		{
			return static_cast<UINT>(m_active_children_.count());
		}

		bool Insert(const WeakT& obj)
		{
			// attempt to insert outside of the map
			if (this == nullptr)
			{
				return false;
			}

			if (!m_b_initialized_)
			{
				m_insertion_queue_.push(obj);
				Build();
				return true;
			}

			// This will be the last node that successfully contains the object
			Octree*             last = nullptr;
			std::stack<Octree*> stack;
			stack.push(this);

			// exact match (which is the smallest node that can contain the object)
			bool found = false;

			const auto bounding_value = BoundingValueGetter::value(obj.lock());

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
							const auto bound       = GetBound(node_extent, node_center, static_cast<eOctant>(i));
							const auto bound_check = bounding_value.ContainsBy(bound);

							if (bound_check == DirectX::ContainmentType::CONTAINS)
							{
								// Expand the node and check same above
								node_children[i]            = std::unique_ptr<Octree>(new Octree{bound});
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

			if (found)
			{
				return true;
			}
			if (!found && last)
			{
				last->m_values_.push_back(obj);
				return true;
			}
			if (!last && bounding_value.ContainsBy(root()->m_bounds_))
			{
				root()->m_values_.push_back(obj);
				return true;
			}

			// there is not match with current object (oob)
			return false;
		}

		void Remove(const WeakT& obj)
		{
			std::stack<Octree*> stack;
			stack.push(this);

			while (!stack.empty())
			{
				const auto node = stack.top();
				stack.pop();

				auto&       node_value           = node->m_values_;
				auto&       node_children        = node->m_children_;
				auto&       node_active_children = node->m_active_children_;
				const auto& node_bound           = node->m_bounds_;
				const auto& node_extent          = node->Extent();
				const auto& node_center          = node->WorldCenter();

				if (!node_value.empty())
				{
					for (auto it = node_value.begin(); it != node_value.end();)
					{
						if (it->expired())
						{
							it = node_value.erase(it);
							continue;
						}

						if (it->lock() == obj.lock())
						{
							it = node_value.erase(it);
							continue;
						}

						++it;
					}
				}

				for (int i = 0; i < octant_count; ++i)
				{
					if (node_children[i])
					{
						if (node_children[i]->garbage())
						{
							node_children[i].reset();
							node_active_children.reset(i);
						}
						else
						{
							stack.push(node_children[i].get());
						}
					}
				}
			}
		}

		void Update()
		{
			unsigned attempt = 0;

			// todo: check whether the node is orphan.

			std::stack<Octree*>     stack;
			std::map<Octree*, bool> visited;

			stack.push(this);

			while (!stack.empty())
			{
				const auto node = stack.top();

				auto&       life_count           = node->m_life_count_;
				auto&       node_value           = node->m_values_;
				auto&       node_children        = node->m_children_;
				auto&       node_active_children = node->m_active_children_;
				auto&       node_bound           = node->m_bounds_;
				const auto& node_extent          = node->Extent();
				const auto& node_center          = node->WorldCenter();

				if (visited.contains(node) && visited[node])
				{
					if (!node_value.empty())
					{
						// Check whether the objects are still bounded by the node
						for (auto it = node_value.begin(); it != node_value.end();)
						{
							if (it->expired())
							{
								it = node_value.erase(it);
								continue;
							}

							const auto obj         = it->lock();
							const auto obj_bound   = BoundingValueGetter::value(obj);
							const auto bound_check = obj_bound.ContainsBy(node_bound);

							if (bound_check != DirectX::ContainmentType::CONTAINS)
							{
								auto* cursor = node->parent();

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
									root()->Panic();
									return;
								}
								// Object moved to parent node, remove from current obj
								it = node_value.erase(it);
								continue;
							}

							++it;
						}
					}

					for (int i = 0; i < octant_count; ++i)
					{
						if (node_children[i])
						{
							if (node_children[i]->garbage())
							{
								node_children[i].reset();
								node_active_children.reset(i);
							}
						}
					}

#if WITH_DEBUG
				{
					GetDebugger().Draw(node_bound, DirectX::Colors::BlanchedAlmond);
				}
#endif
					stack.pop();
					continue;
				}

				if (!visited.contains(node) && !visited[node])
				{
					if (!node->m_b_initialized_)
					{
						node->UpdateInternal();
					}

					while (!node->ready())
					{
						if (attempt > retry_exhaust)
						{
							throw std::logic_error("Octree update failed.");
						}

						node->UpdateInternal();
						++attempt;
					}

					// Lifecycle management
					if (node->empty())
					{
						life_count--;
					}
					else
					{
						life_count = node_lifespan;
					}

					if (!node_value.empty())
					{
						for (auto it = node_value.begin(); it != node_value.end();)
						{
							if (it->expired())
							{
								it = node_value.erase(it);
								continue;
							}

							const auto obj       = it->lock();
							const auto obj_bound = BoundingValueGetter::value(obj);
							bool       found     = false;

							for (int i = 0; i < octant_count; ++i)
							{
								if (node_children[i])
								{
									if (obj_bound.ContainsBy(node_children[i]->m_bounds_) ==
									    DirectX::ContainmentType::CONTAINS)
									{
										// Move the object to the child node for precise boundary
										if (node_children[i]->Insert(obj))
										{
											found = true;
											break;
										}
									}
								}
								else
								{
									const auto bound = GetBound(node_extent, node_center, static_cast<eOctant>(i));
									const auto bound_check = obj_bound.ContainsBy(bound);

									if (bound_check == DirectX::ContainmentType::CONTAINS)
									{
										// Expand the node
										node_children[i]            = std::unique_ptr<Octree>(new Octree{bound});
										node_children[i]->m_parent_ = node;
										node_children[i]->Build();
										node_active_children.set(i);
										if (node_children[i]->Insert(obj))
										{
											found = true;
											break;
										}
									}
								}
							}

							if (found)
							{
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
						if (node_children[i])
						{
							stack.push(node_children[i].get());
						}
					}

					visited[node] = true;
				}
			}
		}

		void Clear()
		{
			std::stack<Octree*> stack;
			stack.push(this);

			while (!stack.empty())
			{
				const auto node = stack.top();
				stack.pop();

				node->m_values_.clear();

				for (int i = 0; i < octant_count; ++i)
				{
					if (node->m_children_[i])
					{
						if (node->m_children_[i]->m_active_children_ == 0 && node->m_values_.size() <= 1)
						{
							node->m_children_[i].reset();
							node->m_active_children_.reset(i);
						}
						else
						{
							stack.push(node->m_children_[i].get());
						}
					}
				}
			}
		}

		void Iterate(const Vector3& point, const std::function<bool(const WeakT&)>& func) const
		{
			std::queue<const Octree*> q;
			q.push(this);

			while (!q.empty())
			{
				const auto node = q.front();
				q.pop();

				const auto& value    = node->m_values_;
				const auto& children = node->m_children_;

				for (const auto& v : value)
				{
					if (func(v))
					{
						return;
					}
				}

				for (const auto& child : children)
				{
					if (child && child->Contains(point))
					{
						q.push(child.get());
					}
				}
			}
		}

		std::vector<WeakT> Nearest(const Vector3& point, float distance) const
		{
			std::stack<const Octree*> q;
			std::set<const Octree*>   visited;
			std::vector<WeakT>        result;
			const BoundingSphere      search_sphere(point, distance);

			q.push(this);

			while (!q.empty())
			{
				const auto node = q.top();

				const auto& value    = node->m_values_;
				const auto& children = node->m_children_;

				if (visited.contains(node))
				{
					q.pop();

					for (const auto& v : value)
					{
						if (const auto& locked = v.lock())
						{
							if (const auto& bounding = BoundingValueGetter::value(*locked);
								bounding.Intersects(search_sphere) || bounding.ContainsBy(search_sphere))
							{
								result.push_back(v);
							}
						}
					}

					continue;
				}

				visited.insert(node);

				for (const auto& child : children)
				{
					if (child && child->Intersects(search_sphere))
					{
						q.push(child.get());
					}
				}
			}

			return result;
		}

		std::vector<WeakT> Hitscan(
			const Vector3& point, const Vector3& direction, size_t count = 0, float distance = 0.f
		) const
		{
			std::stack<const Octree*> q;
			std::set<const Octree*>   visited;
			std::vector<WeakT>        result;
			float                     dist = 0.f;

			q.push(this);

			while (!q.empty())
			{
				const auto node = q.top();

				const auto& value    = node->m_values_;
				const auto& children = node->m_children_;

				if (visited.contains(node))
				{
					q.pop();

					for (const auto& v : value)
					{
						if (const auto& locked = v.lock())
						{
							if (const auto& bounding = BoundingValueGetter::value(*locked);
								bounding.TestRay(point, direction, dist))
							{
								if (!FloatCompare(distance, 0.f))
								{
									if (dist > distance)
									{
										continue;
									}
								}

								result.push_back(v);

								if (count != 0 && result.size() == count)
								{
									break;
								}
							}
						}
					}

					continue;
				}

				visited.insert(node);

				for (const auto& child : children)
				{
					if (child && child->Intersects(point, direction, dist))
					{
						q.push(child.get());
					}
				}
			}

			return result;
		}

	private:
		static bool __vectorcall FloatCompare(const float a, const float b)
		{
			return std::fabs(a - b) <
			       Epsilon * std::fmaxf(1.0f, std::fmaxf(std::fabsf(a), std::fabsf(b)));
		}

		explicit Octree(const BoundingBox& bounds)
			: m_parent_(nullptr),
			  m_b_initialized_(false),
			  m_life_count_(node_lifespan)
		{
			m_bounds_ = bounds;
		}

		explicit Octree(const BoundingBox& bounds, const std::vector<WeakT>& values)
			: m_parent_(nullptr),
			  m_b_initialized_(false),
			  m_life_count_(node_lifespan)
		{
			m_bounds_ = bounds;
			m_values_ = values;
		}

		// Initialize a node
		void Build()
		{
			std::stack<Octree*> stack;
			stack.push(this);

			while (!stack.empty())
			{
				const auto node = stack.top();
				stack.pop();

				auto&      node_value           = node->m_values_;
				auto&      node_children        = node->m_children_;
				auto&      node_active_children = node->m_active_children_;
				auto&      node_active_flag     = node->m_b_initialized_;
				const auto node_extent          = node->Extent();
				const auto node_center          = node->WorldCenter();
				const auto extent_scale         = node->Extent() * 2.f;

				node_active_flag = true;

				if (node_value.size() <= 1 && root() != this)
				{
					continue; // Leaf node
				}

				// Node is bigger than the max map size
				if (extent_scale.x < static_cast<float>(g_max_map_size) &&
				    extent_scale.y < static_cast<float>(g_max_map_size) &&
				    extent_scale.z < static_cast<float>(g_max_map_size))
				{
					continue;
				}

				// Split the node into 8 octants, if it is needed, and distribute the objects
				const std::vector<BoundingBox>               octants = GetBounds(node_extent, node_center);
				std::array<std::vector<WeakT>, octant_count> octant_values;

				for (int i = 0; i < octant_count; ++i)
				{
					for (auto it = node_value.begin(); it != node_value.end();)
					{
						if (BoundingValueGetter::value(it->lock()).ContainsBy(octants[i]) == DirectX::CONTAINS)
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

		// Resolving panic status, rebuilds the whole tree.
		void Panic()
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
						const auto& child = node->m_children_[i];

						if (child->m_active_children_ == 0 && child->m_values_.size() <= 1)
						{
							if (child->m_values_.size() == 1)
							{
								m_insertion_queue_.push(child->m_values_[0]);
							}

							node->m_children_[i].reset();
							node->m_active_children_.reset(i);
						}
						else
						{
							stack.push(node->m_children_[i].get());
						}
					}
				}
			}

			m_values_.clear();

			// Rebuild the tree
			UpdateInternal();
		}

		// Resolving dirty status or rebuilding.
		void UpdateInternal()
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

		// Utility function for getting the octant bounds.
		static std::vector<BoundingBox> __vectorcall GetBounds(const Vector3& extent, const Vector3& center)
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

		static BoundingBox __vectorcall GetBound(const Vector3& extent, const Vector3& center, eOctant region)
		{
			BoundingBox bound;

			switch (region)
			{
			case eOctant::TopLeftFront:
				BoundingBox::CreateFromPoints(bound, center + (extent * tlf), center);
				break;
			case eOctant::TopRightFront:
				BoundingBox::CreateFromPoints(bound, center + (extent * trf), center);
				break;
			case eOctant::BottomLeftFront:
				BoundingBox::CreateFromPoints(bound, center + (extent * blf), center);
				break;
			case eOctant::BottomRightFront:
				BoundingBox::CreateFromPoints(bound, center + (extent * brf), center);
				break;
			case eOctant::TopLeftBack:
				BoundingBox::CreateFromPoints(bound, center, center + (extent * tlb));
				break;
			case eOctant::TopRightBack:
				BoundingBox::CreateFromPoints(bound, center, center + (extent * trb));
				break;
			case eOctant::BottomLeftBack:
				BoundingBox::CreateFromPoints(bound, center, center + (extent * blb));
				break;
			case eOctant::BottomRightBack:
				BoundingBox::CreateFromPoints(bound, center, center + (extent * brb));
				break;
			default:
				throw std::logic_error("Unknown octant value given");
			}

			return bound;
		}

		Octree* root()
		{
			return m_parent_ ? m_parent_->root() : this;
		}

		Octree* parent() const
		{
			return m_parent_ ? m_parent_ : nullptr;
		}

		bool ready() const
		{
			return m_b_initialized_ && !dirty();
		}

		bool dirty() const
		{
			return !m_insertion_queue_.empty();
		}

		bool garbage() const
		{
			return m_life_count_ < 0;
		}

		bool empty() const
		{
			return m_values_.empty() && m_active_children_.none();
		}

		// The max points of the bounding box in world space.
		Vector3 MaxWorldPoint() const
		{
			return Vector3(m_bounds_.Center) + m_bounds_.Extents;
		}

		// The min points of the bounding box in world space.
		Vector3 MinWorldPoint() const
		{
			return Vector3(m_bounds_.Center) - m_bounds_.Extents;
		}

		// The size of the bounding box in world space in half.
		Vector3 Extent() const
		{
			return m_bounds_.Extents;
		}

		// The center of the bounding box in world space.
		Vector3 WorldCenter() const
		{
			return m_bounds_.Center;
		}

		Octree*                                           m_parent_;
		std::array<std::unique_ptr<Octree>, octant_count> m_children_;
		std::bitset<octant_count>                         m_active_children_;
		std::vector<WeakT>                                m_values_;

		BoundingBox m_bounds_;

		std::queue<WeakT> m_insertion_queue_;
		bool              m_b_initialized_;

		int m_life_count_;
	};
}
