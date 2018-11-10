call :LoadVCVars
mkdir build
cd build
cmake .. -G "NMake Makefiles"
nmake
cd ..
goto :EOF

:LoadVCVars
call "%VS150COMNTOOLS%\..\..\VC\vcvarsall.bat"
if not ERRORLEVEL 1 goto DoneLoadVCVars
call "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat"
if not ERRORLEVEL 1 goto DoneLoadVCVars
echo "Visual Studio not found, build will likely fail."
:DoneLoadVCVars
