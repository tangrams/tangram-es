cd %~dp0
::rd /s /q build\windows
mkdir build\windows
cd build\windows
::set VARIANT=Debug
set VARIANT=Release
cmake -G "Visual Studio 14 2015" ..\.. -DPLATFORM_TARGET=windows
cmake --build . --config %VARIANT%
if not exist  %BATOSDIR% goto :end
mkdir %BATOSDIR%\include
mkdir %BATOSDIR%\lib
mkdir %BATOSDIR%\bin
copy %~dp0core\api\tangram-*.h %BATOSDIR%\include
copy bin\*.dll %BATOSDIR%\bin\
copy bin\*.pdb %BATOSDIR%\bin\
copy lib\%VARIANT%\tangram-core.lib %BATOSDIR%\lib\

:end

pause
