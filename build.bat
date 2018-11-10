call :LoadVCVars
mkdir build
cd build
cmake .. -G "NMake Makefiles"
nmake
cd ..
goto :EOF

:: Run the "vsvarsall.bat" script provided by Visual Studio. This will place
:: cl.exe, link.exe, into the path. Currently this is hardcoded to use 64-bit.
:LoadVCVars
call "%VS150COMNTOOLS%\..\..\VC\vcvarsall.bat" x64
if not ERRORLEVEL 1 goto DoneLoadVCVars
call "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" x64
if not ERRORLEVEL 1 goto DoneLoadVCVars
echo "Visual Studio not found, build will likely fail."
:DoneLoadVCVars
