#include <gtest/gtest.h>

#include <crtdbg.h>
#include <cstdlib>
#include <windows.h>

int main( int argc, char** argv )
{
    // Report CRT asserts and crashes on stderr instead of a dialog, so an unattended run ends with a failure.
    _CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
    _CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDERR );
    _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
    _CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDERR );
    _set_abort_behavior( 0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT );
    SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX );

    testing::InitGoogleTest( &argc, argv );
    // Death tests re-run this program for the statement under test (the only mode on Windows).
    GTEST_FLAG_SET( death_test_style, "threadsafe" );
    return RUN_ALL_TESTS();
}
