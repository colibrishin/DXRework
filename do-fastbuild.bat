@echo off

IF NOT "%~1" == "-clean" (
	echo Parsed: %~1 %~2 %~3
	
	FOR /F "tokens=1-3 delims=_" %%A IN ("%~1") DO (
		echo Build Engine_All_%%B_%%C
		start "" /B "./Programs/Sharpmake/tools/FastBuild/Windows-x64/FBuild.exe" Engine_All_%%B_%%C %~2 %~3 -config Intermediate\\ProjectFiles\\Engine.bff
	)
)

IF "%~1" == "-clean" (
	echo Parsed: %~1 %~2 %~3 %~4

	FOR /F "tokens=1-3 delims=_" %%A IN ("%~2") DO (
		echo Rebuild Engine_All_%%B_%%C
		start "" /B "./Programs/Sharpmake/tools/FastBuild/Windows-x64/FBuild.exe" -clean Engine_All_%%B_%%C %FBPARAM1% %FBPARAM2% -config Intermediate\\ProjectFiles\\Engine.bff
	)
)