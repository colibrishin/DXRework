@echo off

IF NOT "%~1" == "-clean" (
	echo Parsed: %~1 %~2 %~3
	
	FOR /F "tokens=1-3 delims=_" %%A IN ("%~1") DO (
		if "%%B" == "All" (
			echo Build All
		) else (
			echo Build %~1
		)

		start "" /B "./Programs/Sharpmake/tools/FastBuild/Windows-x64/FBuild.exe" %~1 %~2 %~3 -config %%A.bff -ide -nofastcancel -wrapper
	)
)

IF "%~1" == "-clean" (
	echo Parsed: %~1 %~2 %~3 %~4

	echo Cleaning up genereted headers...
	rmdir /S /Q "./Intermediate/HeaderParser/"

	FOR /F "tokens=1-3 delims=_" %%A IN ("%~2") DO (
		if "%%B" == "All" (
			echo Rebuild All
		) else (
			echo Build %~1
		)

		start "" /B "./Programs/Sharpmake/tools/FastBuild/Windows-x64/FBuild.exe" -clean %~2 %FBPARAM1% %FBPARAM2% -config %%A.bff -ide -nofastcancel -wrapper
	)
)