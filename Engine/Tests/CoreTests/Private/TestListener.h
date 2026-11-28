#pragma once
#include <functional>

#include "Entity.h"

#include "TestListener.generated.h"

namespace Engine::Tests
{
    // An entity with a member function to register on a Delegate. OnSignal counts its calls, adds the value and then
    // runs action, so a test can remove or broadcast from inside a callback.
    ECLASS()
    class TestListener : public Engine::Abstracts::Entity
    {
    public:
        GENERATE_BODY

        TestListener() = default;

        void OnSignal( int value )
        {
            ++calls;
            sum += value;
            if ( action )
            {
                action();
            }
        }

        void PreUpdate( const float ) override {}
        void Update( const float ) override {}
        void PostUpdate( const float ) override {}
        void FixedUpdate( const float ) override {}
        void OnSerialized() override {}
        void OnDeserialized() override {}

        int                   calls = 0;
        int                   sum   = 0;
        std::function<void()> action;
    };
}
