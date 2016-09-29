cd %~dp0
::rd /s /q build\windows
mkdir build\windows
cd build\windows
if not exist  %BATOSDIR% goto :end

cmake -G "Visual Studio 14 2015" ..\.. -DPLATFORM_TARGET=windows
mkdir %BATOSDIR%\include
mkdir %BATOSDIR%\lib
mkdir %BATOSDIR%\bin
copy %~dp0core\api\tangram-*.h %BATOSDIR%\include

for %%v in (debug release) do call :install %%v

:end

pause


goto :eof
:install
echo Installing with variant %1
set VARIANT=%1
cmake --build . --config %VARIANT%

set BIN_DIR=%BATOSDIR%\bin\%VARIANT%
set LIB_DIR=%BATOSDIR%\lib\%VARIANT%

mkdir %BIN_DIR%
mkdir %LIB_DIR%

copy bin\tangram-core.dll %BIN_DIR%
copy bin\tangram-core.pdb %BIN_DIR%
copy lib\%VARIANT%\tangram-core.lib %LIB_DIR%

goto :eof
