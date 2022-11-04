@echo off

RMDIR dist /S /Q

cmake -S . --preset=skyrim --check-stamp-file "build\skyrim\CMakeFiles\generate.stamp"
#if %ERRORLEVEL% NEQ 0 exit 1
cmake -DCMAKE_TOOLCHAIN_FILE=F:\GithubMods\DLSS\vcpkg\scripts\buildsystems\vcpkg.cmake --build build/skyrim --config Release
#if %ERRORLEVEL% NEQ 0 exit 1

xcopy "build\skyrim\release\*.dll" "dist\SKSE\Plugins\" /I /Y
xcopy "build\skyrim\release\*.pdb" "dist\SKSE\Plugins\" /I /Y

xcopy "package" "dist" /I /Y /E

pause