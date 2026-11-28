#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <random>
#include <set>
#include <vector>

#include "GenericBounding.hpp"
#include "Octree.hpp"

// The position octree with plain objects: each object is only its bounding box. A query on the tree must return
// exactly what testing every live object returns, and every live object must be in the tree once.
namespace
{
    struct FakeObject
    {
        Engine::GenericBounding<> bounding;
        Vector3                   center;
    };

    using Strong = std::shared_ptr<FakeObject>;
    using Weak   = std::weak_ptr<FakeObject>;

    struct FakeGetter
    {
        static Engine::GenericBounding<> value( const Strong& object )
        {
            return object->bounding;
        }
    };

    using Tree = Engine::octree_impl<Weak, FakeGetter>;

    struct World
    {
        std::mt19937          random{ 1234 };
        std::vector<Strong>   objects;
        float                 sphere_share = 0.f; // share of objects with a sphere bounding

        float Uniform( const float lo, const float hi )
        {
            return std::uniform_real_distribution<float>( lo, hi )( random );
        }

        void Place( FakeObject& object, const float range )
        {
            object.center = Vector3( Uniform( -range, range ), Uniform( -range, range ), Uniform( -range, range ) );
            const Vector3 extents( Uniform( 0.2f, 5.f ), Uniform( 0.2f, 5.f ), Uniform( 0.2f, 5.f ) );
            if ( Uniform( 0.f, 1.f ) < sphere_share )
            {
                object.bounding.SetType( Engine::BOUNDING_TYPE_SPHERE );
                object.bounding.SetCenter( object.center );
                object.bounding.SetExtents( extents );
                return;
            }
            object.bounding.SetType( Engine::BOUNDING_TYPE_BOX );
            object.bounding.UpdateFromBoundingBox(
                    DirectX::BoundingOrientedBox( object.center, extents, Quaternion::Identity ) );
        }

        void Move( FakeObject& object, const float step )
        {
            object.center += Vector3( Uniform( -step, step ), Uniform( -step, step ), Uniform( -step, step ) );
            object.bounding.SetCenter( object.center );
        }

        Strong Add( Tree& tree, const float range )
        {
            auto object = std::make_shared<FakeObject>();
            Place( *object, range );
            objects.push_back( object );
            tree.Insert( object );
            return object;
        }
    };

    // Objects in the root or its children (an object deeper than that is away from the map's center planes).
    size_t ShallowObjects( const Tree& tree )
    {
        size_t count = tree.Read().size();
        for ( const Tree* child : tree.Next() )
        {
            if ( child )
            {
                count += child->Read().size();
            }
        }
        return count;
    }

    // Every live object exactly once, nothing expired, and queries equal to testing every object.
    void ExpectConsistent( const Tree& tree, World& world, const int queries = 64 )
    {
        std::multiset<const FakeObject*> in_tree;
        std::vector<const Tree*>         nodes{ &tree };
        size_t                           outside = 0;
        while ( !nodes.empty() )
        {
            const Tree* node = nodes.back();
            nodes.pop_back();
            for ( const Weak& value : node->Read() )
            {
                const Strong locked = value.lock();
                ASSERT_TRUE( locked ) << "expired object left in the tree";
                in_tree.insert( locked.get() );
                // The root also keeps what is outside the map; any other node contains its objects.
                if ( node != &tree && !node->Contains( locked->bounding.GetBoxAxisAlignedBounds() ) )
                {
                    ++outside;
                }
            }
            for ( const Tree* child : node->Next() )
            {
                if ( child )
                {
                    nodes.push_back( child );
                }
            }
        }

        EXPECT_EQ( outside, 0u ) << "objects in a node that does not contain them";

        std::multiset<const FakeObject*> live;
        for ( const Strong& object : world.objects )
        {
            live.insert( object.get() );
        }
        if ( in_tree != live )
        {
            size_t missing = 0, duplicated = 0, foreign = 0;
            for ( const FakeObject* object : live )
            {
                const size_t count = in_tree.count( object );
                missing += count == 0;
                duplicated += count > 1 ? count - 1 : 0;
            }
            for ( const FakeObject* object : in_tree )
            {
                foreign += live.count( object ) == 0;
            }
            FAIL() << "tree " << in_tree.size() << " live " << live.size() << ": missing " << missing
                   << ", duplicated " << duplicated << ", not live " << foreign;
        }

        for ( int q = 0; q < queries; ++q )
        {
            Vector3 point( world.Uniform( -350.f, 350.f ), world.Uniform( -350.f, 350.f ),
                           world.Uniform( -350.f, 350.f ) );
            float   radius = world.Uniform( 1.f, 60.f );
            if ( q % 2 && !world.objects.empty() )
            {
                // Just outside an object: finds an object that sticks out of its node.
                const FakeObject& probe = *world.objects[ static_cast<size_t>(
                        world.Uniform( 0.f, static_cast<float>( world.objects.size() ) - 0.01f ) ) ];
                Vector3 offset( world.Uniform( -1.f, 1.f ), world.Uniform( -1.f, 1.f ), world.Uniform( -1.f, 1.f ) );
                offset.Normalize();
                const Vector3 extents = probe.bounding.GetExtents();
                point  = probe.center + offset * ( std::max( { extents.x, extents.y, extents.z } ) * 1.2f );
                radius = world.Uniform( 0.5f, 2.f );
            }

            std::set<const FakeObject*> expected;
            const DirectX::BoundingSphere sphere( point, radius );
            for ( const Strong& object : world.objects )
            {
                if ( object->bounding.Intersects( sphere ) || object->bounding.ContainsBy( sphere ) )
                {
                    expected.insert( object.get() );
                }
            }
            std::set<const FakeObject*> nearest;
            for ( const Weak& value : tree.Nearest( point, radius ) )
            {
                nearest.insert( value.lock().get() );
            }
            EXPECT_EQ( nearest, expected ) << "Nearest " << q;

            Vector3 direction( world.Uniform( -1.f, 1.f ), world.Uniform( -1.f, 1.f ), world.Uniform( -1.f, 1.f ) );
            direction.Normalize();
            expected.clear();
            for ( const Strong& object : world.objects )
            {
                float distance = 0.f;
                if ( object->bounding.TestRay( point, direction, distance ) )
                {
                    expected.insert( object.get() );
                }
            }
            std::set<const FakeObject*> hits;
            for ( const Weak& value : tree.Hitscan( point, direction ) )
            {
                hits.insert( value.lock().get() );
            }
            EXPECT_EQ( hits, expected ) << "Hitscan " << q;
        }
    }
}

TEST( GenericBounding, AxisAlignedBoundsOfASphereContainTheSphere )
{
    Engine::GenericBounding<> bounding;
    bounding.SetType( Engine::BOUNDING_TYPE_SPHERE );
    bounding.SetCenter( Vector3( 10.f, -4.f, 2.f ) );
    bounding.SetExtents( Vector3( 3.f ) );

    const BoundingBox aabb = bounding.GetBoxAxisAlignedBounds();
    EXPECT_EQ( aabb.Contains( DirectX::BoundingSphere( Vector3( 10.f, -4.f, 2.f ), 3.f ) ), DirectX::CONTAINS )
        << "center " << aabb.Center.x << " " << aabb.Center.y << " " << aabb.Center.z << " extents " << aabb.Extents.x
        << " " << aabb.Extents.y << " " << aabb.Extents.z;
}

TEST( GenericBounding, ContainsByTestsTheBoundingOfItsType )
{
    // A box whose first floats, read as a sphere (center, radius = extents.x), would reach the sphere below.
    Engine::GenericBounding<> box;
    box.SetType( Engine::BOUNDING_TYPE_BOX );
    box.UpdateFromBoundingBox( DirectX::BoundingOrientedBox( Vector3( 0.f, 0.f, 0.f ), Vector3( 3.f, 3.f, 0.5f ),
                                                             Quaternion::Identity ) );
    const DirectX::BoundingSphere below( Vector3( 0.f, 0.f, -2.f ), 1.f );
    EXPECT_FALSE( box.Intersects( below ) );
    EXPECT_EQ( box.ContainsBy( below ), DirectX::DISJOINT );

    Engine::GenericBounding<> sphere;
    sphere.SetType( Engine::BOUNDING_TYPE_SPHERE );
    sphere.SetCenter( Vector3( 0.f, 0.f, 0.f ) );
    sphere.SetExtents( Vector3( 1.f ) );
    EXPECT_EQ( sphere.ContainsBy( DirectX::BoundingSphere( Vector3( 0.f, 0.f, 0.f ), 2.f ) ), DirectX::CONTAINS );
    EXPECT_EQ( sphere.ContainsBy( DirectX::BoundingOrientedBox( Vector3( 0.f, 0.f, 0.f ), Vector3( 2.f, 2.f, 2.f ),
                                                                Quaternion::Identity ) ),
               DirectX::CONTAINS );
}

TEST( Octree, InsertThenUpdatePlacesEveryObject )
{
    World world;
    Tree  tree;
    for ( int i = 0; i < 2000; ++i )
    {
        world.Add( tree, 300.f );
    }
    tree.Update();
    ExpectConsistent( tree, world );
}

TEST( Octree, UpdateFollowsMovedObjects )
{
    World world;
    Tree  tree;
    for ( int i = 0; i < 2000; ++i )
    {
        world.Add( tree, 300.f );
    }
    tree.Update();

    for ( int frame = 0; frame < 40; ++frame )
    {
        for ( const Strong& object : world.objects )
        {
            if ( world.Uniform( 0.f, 1.f ) < 0.2f )
            {
                // Mostly small steps (stays in or near its node), sometimes anywhere.
                if ( world.Uniform( 0.f, 1.f ) < 0.8f )
                {
                    world.Move( *object, 3.f );
                }
                else
                {
                    world.Place( *object, 300.f );
                }
            }
        }
        tree.Update();
        ExpectConsistent( tree, world, 16 );
    }
}

TEST( Octree, SphereObjectsArePlacedByTheirSphere )
{
    World world;
    world.sphere_share = 0.5f;
    Tree  tree;
    for ( int i = 0; i < 2000; ++i )
    {
        world.Add( tree, 300.f );
    }
    tree.Update();

    for ( int frame = 0; frame < 10; ++frame )
    {
        for ( const Strong& object : world.objects )
        {
            if ( world.Uniform( 0.f, 1.f ) < 0.2f )
            {
                // Mostly small steps (stays in or near its node), sometimes anywhere.
                if ( world.Uniform( 0.f, 1.f ) < 0.8f )
                {
                    world.Move( *object, 3.f );
                }
                else
                {
                    world.Place( *object, 300.f );
                }
            }
        }
        tree.Update();
        ExpectConsistent( tree, world, 16 );
    }
}

TEST( Octree, NearestReturnsTheNearestCenterFirst )
{
    World world;
    world.sphere_share = 0.3f;
    Tree  tree;
    for ( int i = 0; i < 2000; ++i )
    {
        world.Add( tree, 100.f );
    }
    tree.Update();

    for ( int q = 0; q < 32; ++q )
    {
        const Vector3 point( world.Uniform( -100.f, 100.f ), world.Uniform( -100.f, 100.f ),
                             world.Uniform( -100.f, 100.f ) );
        const std::vector<Weak> nearest = tree.Nearest( point, 30.f );
        ASSERT_FALSE( nearest.empty() );
        float last = 0.f;
        for ( const Weak& value : nearest )
        {
            const float distance = Vector3::DistanceSquared( value.lock()->center, point );
            EXPECT_GE( distance, last );
            last = distance;
        }
    }
}

TEST( Octree, ObjectsOutsideTheMapStayInTheTree )
{
    World world;
    Tree  tree;
    for ( int i = 0; i < 500; ++i )
    {
        world.Add( tree, 300.f );
    }
    tree.Update();

    for ( int frame = 0; frame < 10; ++frame )
    {
        // A few objects leave the map (+-1024), a few come back.
        for ( int i = 0; i < 5; ++i )
        {
            FakeObject& object = *world.objects[ static_cast<size_t>( world.Uniform( 0.f, 499.f ) ) ];
            if ( frame % 3 == 2 )
            {
                world.Place( object, 300.f );
                continue;
            }
            const DirectX::BoundingOrientedBox away( Vector3( world.Uniform( 1500.f, 3000.f ), 0.f, 0.f ),
                                                     Vector3( 1.f, 1.f, 1.f ), Quaternion::Identity );
            object.bounding.UpdateFromBoundingBox( away );
        }
        const size_t shallow_before = ShallowObjects( tree );
        tree.Update();
        SCOPED_TRACE( frame );
        ExpectConsistent( tree, world, 16 );
        // Leaving the map does not collapse the tree: the other objects stay in their deep nodes.
        EXPECT_LE( ShallowObjects( tree ), shallow_before + 5 );
        if ( HasFatalFailure() )
        {
            return;
        }
    }
}

TEST( Octree, RemovedAndExpiredObjectsLeave )
{
    World world;
    Tree  tree;
    for ( int i = 0; i < 1000; ++i )
    {
        world.Add( tree, 300.f );
    }
    tree.Update();

    for ( int frame = 0; frame < 20; ++frame )
    {
        for ( size_t i = 0; i < world.objects.size(); )
        {
            const float roll = world.Uniform( 0.f, 1.f );
            if ( roll < 0.03f )
            {
                tree.Remove( world.objects[ i ] );
                world.objects.erase( world.objects.begin() + i );
                continue;
            }
            if ( roll < 0.05f )
            {
                // Destroyed without Remove: the next Update drops it.
                world.objects.erase( world.objects.begin() + i );
                continue;
            }
            ++i;
        }
        for ( int i = 0; i < 30; ++i )
        {
            world.Add( tree, 300.f );
        }
        tree.Update();
        ExpectConsistent( tree, world, 16 );
    }
}
