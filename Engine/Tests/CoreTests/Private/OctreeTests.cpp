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

        float Uniform( const float lo, const float hi )
        {
            return std::uniform_real_distribution<float>( lo, hi )( random );
        }

        void Place( FakeObject& object, const float range )
        {
            const DirectX::BoundingOrientedBox box(
                    Vector3( Uniform( -range, range ), Uniform( -range, range ), Uniform( -range, range ) ),
                    Vector3( Uniform( 0.2f, 5.f ), Uniform( 0.2f, 5.f ), Uniform( 0.2f, 5.f ) ),
                    Quaternion::Identity );
            object.bounding.SetType( Engine::BOUNDING_TYPE_BOX );
            object.bounding.UpdateFromBoundingBox( box );
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

    // Every live object exactly once, nothing expired, and queries equal to testing every object.
    void ExpectConsistent( const Tree& tree, World& world, const int queries = 64 )
    {
        std::multiset<const FakeObject*> in_tree;
        std::vector<const Tree*>         nodes{ &tree };
        while ( !nodes.empty() )
        {
            const Tree* node = nodes.back();
            nodes.pop_back();
            for ( const Weak& value : node->Read() )
            {
                const Strong locked = value.lock();
                ASSERT_TRUE( locked ) << "expired object left in the tree";
                in_tree.insert( locked.get() );
            }
            for ( const Tree* child : node->Next() )
            {
                if ( child )
                {
                    nodes.push_back( child );
                }
            }
        }

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
            const Vector3 point( world.Uniform( -350.f, 350.f ), world.Uniform( -350.f, 350.f ),
                                 world.Uniform( -350.f, 350.f ) );
            const float   radius = world.Uniform( 1.f, 60.f );

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
