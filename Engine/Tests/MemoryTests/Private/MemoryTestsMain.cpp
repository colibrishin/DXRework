#include <gtest/gtest.h>

int main( int argc, char** argv )
{
    testing::InitGoogleTest( &argc, argv );
    // Death tests re-run this program for the statement under test (the only mode on Windows).
    GTEST_FLAG_SET( death_test_style, "threadsafe" );
    return RUN_ALL_TESTS();
}
